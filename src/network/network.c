#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "config/config.h"
#include "platform/platform_input.h"
#include "platform/platform_threads.h"
#include "utils/error.h"
#include "graphics/animation.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
// Windows socket compatibility
#define close closesocket
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

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

static bongocat_error_t network_reconnect(void) {
    static time_t last_reconnect_attempt = 0;
    time_t now = time(NULL);
    if (now - last_reconnect_attempt < 5) {
        bongocat_log_info("Reconnect throttled, try again later");
        return BONGOCAT_ERROR_NETWORK;
    }
    last_reconnect_attempt = now;

    bongocat_log_info("Attempting to reconnect...");

    if (sock > 0) {
        close(sock);
        sock = 0;
    }

    struct sockaddr_in *ip_of_server = &current_config->server_address;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        bongocat_log_error("Socket not created during reconnect\n");
        return BONGOCAT_ERROR_NETWORK;
    }

    if (connect(sock, (struct sockaddr *)ip_of_server, sizeof(*ip_of_server)) < 0) {
        bongocat_log_error("Reconnection failed\n");
        close(sock);
        sock = 0;
        return BONGOCAT_ERROR_NETWORK;
    }

    bongocat_log_info("Reconnection successful");
    return BONGOCAT_SUCCESS;
}

static void *network_handle_key_press_thread_main(void *arg __attribute__((unused)))
{
  bongocat_log_info("Starting network handle key press thread");
  int fd = sock;

  while (run_network) {
    if (atomic_load(any_key_pressed)) {
      int key_code = atomic_load(last_key_code);
      int active_frame = get_frame_for_keycode(key_code);
      char* hand = "left";
      if (active_frame == 2) {
        hand = "right";
      }

      bongocat_log_info("send - handle key press thread");
      char msg[64];
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      long long millis = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
      snprintf(msg, sizeof(msg), "key pressed (%s) at %lld", hand, millis);
      if (send(fd, msg, strlen(msg), 0) < 0) {
        bongocat_log_error("Send failed, attempting reconnect\n");
        if (network_reconnect() == BONGOCAT_SUCCESS) {
          fd = sock;
        } else {
          bongocat_log_error("Reconnection failed, giving up on this send\n");
        }
      }

      // 0.1s
      usleep(100 * 1000);
    }
  }

  bongocat_log_info("Exiting network handle key press thread");
  return NULL;

}

bongocat_error_t network_init(config_t *config) {
    BONGOCAT_CHECK_NULL(config, BONGOCAT_ERROR_INVALID_PARAM);
    run_network = true;

    current_config = config;
    bongocat_log_info("Initializing network system");

    // Note: WSAStartup is now called in main() on Windows (needed for DNS during config parsing)

    struct sockaddr_in *ip_of_server = &current_config->server_address;
    if (ip_of_server == NULL) {
        bongocat_log_error("Server address is NULL\n");
        return BONGOCAT_ERROR_INVALID_PARAM;
    }

    // Check if server address is actually configured (not 0.0.0.0:0)
    if (ip_of_server->sin_addr.s_addr == 0 || ip_of_server->sin_port == 0) {
        bongocat_log_info("No network server configured (server_address not set in config)");
        return BONGOCAT_ERROR_NETWORK;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        bongocat_log_error("Socket not created\n");
        return BONGOCAT_ERROR_NETWORK;
    }

    bongocat_log_info("Connecting to server: %s:%d", 
                     inet_ntoa(ip_of_server->sin_addr),
                     ntohs(ip_of_server->sin_port));
    
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

#ifdef _WIN32
  WSACleanup();
#endif

  bongocat_log_debug("Network cleanup complete");
}
