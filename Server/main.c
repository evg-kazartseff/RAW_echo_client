//
// Created by evgenii on 22.01.18.
//

#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <stdlib.h>


int run = 1;

void interapt(int signo) {
    if (signo == SIGINT) {
        run = 0;
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        perror("Use UDP_Server \"IP address\" \"port\"");
        return 1;
    }
    signal(SIGINT, interapt);
    int sd_listener;
    if ((sd_listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error: Con't create socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    struct in_addr ip_addr;
    memset(&ip_addr, 0, sizeof(struct in_addr));
    if(inet_aton(argv[1], &ip_addr) == 0) {
        perror("Error: Can't fill ip address");
        close(sd_listener);
        return 1;
    };
    addr.sin_addr = ip_addr;
    long port = strtol(argv[2], NULL, 10);
    if (port < 0) {
        perror("Error: Port can't be negative");
        close(sd_listener);
        return 1;
    } else if (port > USHRT_MAX) {
        perror("Error: Port can't be more than 65535");
        close(sd_listener);
        return 1;
    }
    addr.sin_port = htons((uint16_t) port);
    if(bind(sd_listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error: Con't bind socket");
        return 1;
    }
    struct sockaddr client;
    memset(&client, 0, sizeof(struct sockaddr_in));
    ssize_t bytes_read;
    char* buff = malloc(1024 * sizeof(char));
    socklen_t socklen = sizeof(struct sockaddr);
    while (run) {
        bytes_read = recvfrom(sd_listener, buff, 1024, 0, &client, &socklen);
        printf("recv\n");
        if (bytes_read <= 0)
            break;
        buff[bytes_read] = '\000';
        if (bytes_read <= 1019) {
            strcat(buff, " - ok");
            bytes_read += 5;
        }
        sendto(sd_listener, buff, (size_t) bytes_read, 0, &client, socklen);
        printf("send\n");
    }
    if (sd_listener > 0)
        close(sd_listener);
    if (buff)
        free(buff);
    return 0;
}