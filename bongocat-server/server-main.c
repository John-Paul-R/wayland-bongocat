#include <stdio.h> // standard input and output library
#include <stdlib.h> // this includes functions regarding memory allocation
#include <string.h> // contains string functions
#include <errno.h> //It defines macros for reporting and retrieving error conditions through error codes
#include <time.h> //contains various functions for manipulating date and time
#include <unistd.h> //contains various constants
#include <sys/types.h> //contains a number of basic derived types that should be used whenever appropriate
#include <arpa/inet.h> // defines in_addr structure
#include <sys/socket.h> // for socket creation
#include <netinet/in.h> //contains constants and structures needed for internet domain addresses

int main()
{
    time_t clock;
    char data_sending[1025];
    int sock_listen_clients_fd = 0;
    int sock_client_conn_fd = 0;
    struct sockaddr_in ip_of_server;
    sock_listen_clients_fd = socket(AF_INET, SOCK_STREAM, 0); // creating socket

    memset(&ip_of_server, 0, sizeof(ip_of_server));
    memset(data_sending, '0', sizeof(data_sending));

    ip_of_server.sin_family = AF_INET;
    ip_of_server.sin_addr.s_addr = htonl(INADDR_ANY);
    ip_of_server.sin_port = htons(2017);         // this is the port number of running server

    if (bind(sock_listen_clients_fd, (struct sockaddr*)&ip_of_server , sizeof(ip_of_server)) != 0) {
      // err
    }
    listen(sock_listen_clients_fd , 20);

    while(1)
    {
        printf("Hi, I am running server. Some Client hit me\n"); // whenever a request from client came. It will be processed here.
        sock_client_conn_fd = accept(sock_listen_clients_fd, (struct sockaddr*)NULL, NULL);

        clock = time(NULL);
        snprintf(data_sending, sizeof(data_sending), "%.24s\r\n", ctime(&clock)); // Printing successful message
        write(sock_client_conn_fd, data_sending, strlen(data_sending));

        close(sock_client_conn_fd);
        sleep(1);
    }

    return 0;
}
