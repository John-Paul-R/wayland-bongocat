#include "config/config.h"
#include "platform/input.h"
#include "utils/error.h"
#include <stdatomic.h>
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

static config_t *current_config;
static int sock = 0;

static pthread_t recv_thread;
static pthread_t send_thread;
static bool run_network = false;
static bool recv_thread_started = false;
static bool send_thread_started = false;
static bool network_initialized = false;

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
            bongocat_log_error("Send failed\n");
            break;
        }
        sleep(3);
    }
    return NULL;
}

static void *network_handle_key_press_thread_main(void *arg __attribute__((unused)))
{
    // int fd = sock;
    // const char *msg = "key pressed";

    // while (run_network) {
    //   if (atomic_load(any_key_pressed)) {
    //     send(fd, msg, strlen(msg), 0);
    //     // 0.25s
    //     // usleep(250 * 1000);
    //     sleep(1);
    //   }
    // }

    return NULL;
}

bongocat_error_t network_init(config_t *config) {
    BONGOCAT_CHECK_NULL(config, BONGOCAT_ERROR_INVALID_PARAM);
    run_network = true;

    current_config = config;
    bongocat_log_info("Initializing network system");

    struct sockaddr_in *ip_of_server = &current_config->server_address;
    if (ip_of_server == NULL) {
        bongocat_log_error("Server address is NULL\n");
        return BONGOCAT_ERROR_INVALID_PARAM;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        bongocat_log_error("Socket not created\n");
        return BONGOCAT_ERROR_NETWORK;
    }

    printf("Server address: %s\n", inet_ntoa(ip_of_server->sin_addr));
    if (connect(sock, (struct sockaddr *)ip_of_server, sizeof(*ip_of_server)) < 0)
    {
        bongocat_log_error("Connection failed due to port and ip problems\n");
        return BONGOCAT_ERROR_NETWORK;
    }

    // // pthread_create(&recv_thread, NULL, receiver, NULL);
    pthread_create(&send_thread, NULL, network_handle_key_press_thread_main, NULL);
    send_thread_started = true;

    network_initialized = true;

    return BONGOCAT_SUCCESS;
}


void network_cleanup(void) {
  run_network = false;
  if (recv_thread_started) {
    bongocat_log_debug("Stopping recv thread");

    // Wait for thread to finish gracefully
    pthread_join(recv_thread, NULL);
    recv_thread_started = false;
    bongocat_log_debug("recv thread stopped");
  }

  if (send_thread_started) {
    bongocat_log_debug("Stopping send thread");

    pthread_join(send_thread, NULL);
    send_thread_started = false;
    bongocat_log_debug("send thread stopped");
  }

  if (network_initialized) {
    network_initialized = false;
  }

  bongocat_log_debug("Network cleanup complete");
}
