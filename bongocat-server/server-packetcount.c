#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "hashmap.h"

// ---- hashmap entry and supporting functions ----

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int count;
} client_entry;

static uint64_t entry_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const client_entry *e = item;
    return hashmap_murmur(e->ip, strlen(e->ip), seed0, seed1);
}

static int entry_compare(const void *a, const void *b, void *udata) {
    return strcmp(((client_entry *)a)->ip, ((client_entry *)b)->ip);
}

// ---- shared state ----

static struct hashmap *client_map;
static pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

static int *active_fds = NULL;
static int active_fds_count = 0;
static int active_fds_cap = 0;
static pthread_mutex_t fds_mutex = PTHREAD_MUTEX_INITIALIZER;

static void fds_add(int fd) {
    pthread_mutex_lock(&fds_mutex);
    if (active_fds_count == active_fds_cap) {
        active_fds_cap = active_fds_cap == 0 ? 4 : active_fds_cap * 2;
        active_fds = realloc(active_fds, active_fds_cap * sizeof(int));
    }
    active_fds[active_fds_count++] = fd;
    pthread_mutex_unlock(&fds_mutex);
}

static void fds_remove(int fd) {
    pthread_mutex_lock(&fds_mutex);
    for (int i = 0; i < active_fds_count; i++) {
        if (active_fds[i] == fd) {
            active_fds[i] = active_fds[--active_fds_count]; // swap with last
            break;
        }
    }
    pthread_mutex_unlock(&fds_mutex);
}

static void broadcast(const char *msg, int sender_fd) {
    pthread_mutex_lock(&fds_mutex);
    for (int i = 0; i < active_fds_count; i++) {
        if (active_fds[i] != sender_fd) // don't echo back to sender
            write(active_fds[i], msg, strlen(msg));
    }
    pthread_mutex_unlock(&fds_mutex);
}

// ---- per-connection thread ----

typedef struct {
    int fd;
    char ip[INET_ADDRSTRLEN];
} conn_args;

static void *handle_client(void *arg) {
    printf("handle_client start\n");
    conn_args *args = arg;
    int fd = args->fd;
    char ip[INET_ADDRSTRLEN];
    strncpy(ip, args->ip, INET_ADDRSTRLEN);
    fds_add(fd);
    free(args);

    char buf[1025];
    ssize_t n;

    // Read packets until the client closes the connection
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';

        // Increment packet count for this IP
        pthread_mutex_lock(&map_mutex);
        client_entry lookup = { .count = 0 };
        strncpy(lookup.ip, ip, INET_ADDRSTRLEN);
        const client_entry *existing = hashmap_get(client_map, &lookup);
        if (existing) {
            lookup.count = existing->count + 1;
        } else {
            lookup.count = 1;
        }
        hashmap_set(client_map, &lookup);
        int current_count = lookup.count;
        pthread_mutex_unlock(&map_mutex);

        // Respond with a timestamped acknowledgement
        time_t clock = time(NULL);
        char response[1100];
        snprintf(response, sizeof(response),
            "%.24s | %s | packets received: %d\r\n",
            ctime(&clock), ip, current_count);
        write(fd, response, strlen(response));

        // notify everyone else
        char notice[1200];
        snprintf(notice, sizeof(notice),
            "[broadcast] %s sent a packet (their total: %d)\r\n", ip, current_count);
        broadcast(notice, fd);
    }

    printf("Client %s disconnected\n", ip);
    fds_remove(fd);
    close(fd);
    return NULL;
}

// ---- main ----

int main() {
    client_map = hashmap_new(
        sizeof(client_entry), 16,
        0, 0,
        entry_hash, entry_compare,
        NULL, NULL
    );

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // Allow port reuse so restarting the server doesn't fail with EADDRINUSE
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(2017);

    bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listenfd, 20);

    printf("Server listening on port 2017\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        printf("Waiting for a client to connect...\n");
        int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len);
        printf("'accept' resolved.\n");
        if (connfd < 0) continue;

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("New connection from %s\n", client_ip);

        conn_args *args = malloc(sizeof(conn_args));
        args->fd = connfd;
        strncpy(args->ip, client_ip, INET_ADDRSTRLEN);

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, args);
        pthread_detach(thread);  // Let the thread clean itself up when done
    }

    hashmap_free(client_map);
    return 0;
}
