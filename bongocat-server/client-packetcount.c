#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>

static int sock = 0;

static void *receiver(void *arg) {
    char buf[1024];
    int n;
    while ((n = read(sock, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = 0;
        fputs(buf, stdout);
        printf("\n");
    }
    return NULL;
}

static void *sender(void *arg) {
    const char *ping = "ping\r\n";
    while (1) {
        if (write(sock, ping, strlen(ping)) < 0) {
            printf("Send failed\n");
            break;
        }
        sleep(3);
    }
    return NULL;
}

int main()
{
    struct sockaddr_in ipOfServer;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket not created\n");
        return 1;
    }

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_port = htons(2017);
    ipOfServer.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&ipOfServer, sizeof(ipOfServer)) < 0)
    {
        printf("Connection failed due to port and ip problems\n");
        return 1;
    }

    pthread_t recv_thread, send_thread;
    pthread_create(&recv_thread, NULL, receiver, NULL);
    pthread_create(&send_thread, NULL, sender, NULL);

    pthread_join(recv_thread, NULL); // block until server closes connection
    close(sock);
    return 0;
}
