//
// Created by evgenii on 22.01.18.
//

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <stdlib.h>

struct ip_header {
#if BYTE_ORDER == LITTLE_ENDIAN
    u_int ip_hl:4, ip_v:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    u_int ip_v:4, /* version */
	ip_hl:4; /* header length */
#endif
    u_char ip_tos;
    u_short ip_len;
    u_short ip_id;
    u_short ip_off;
    u_char ip_ttl;
    u_char ip_p;
    u_short ip_sum;
    struct in_addr ip_src;
    struct in_addr ip_dst;
};

struct udp_header {
    u_short src;
    u_short dst;
    u_short len;
    u_short check;
};

int main(int argc, char** argv) {
    if (argc != 3) {
        perror("Use TCP_Client \"IP address\" \"port\"");
        return 1;
    }
    int sock;
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
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
        close(sock);
        return 1;
    };
    addr.sin_addr = ip_addr;
    long port = strtol(argv[2], NULL, 10);
    if (port < 0) {
        perror("Error: Port can't be negative");
        close(sock);
        return 1;
    } else if (port > USHRT_MAX) {
        perror("Error: Port can't be more than 65535");
        close(sock);
        return 1;
    }
    addr.sin_port = htons((uint16_t) port);

    uint16_t my_port = 5456;

    size_t data_len = 11;
    char data[data_len];
    memcpy(data, "Hello world", data_len);

    size_t size_udp_header = sizeof(struct udp_header);
    size_t size_udp_packet = sizeof(struct udp_header) + data_len;

    char* buff = malloc(size_udp_packet * sizeof(char));
    memset(buff, 0, size_udp_packet);
    struct udp_header* udp = (struct udp_header*) buff;
    udp->src = htons(my_port);
    udp->dst = htons((uint16_t) port);
    udp->len = htons((uint16_t) size_udp_packet);
    udp->check = 0;
    memcpy(buff +  sizeof(struct udp_header), data, data_len);
    if (sendto(sock, buff, size_udp_packet, 0, (struct sockaddr*) &addr, sizeof(struct sockaddr_in)) < 0) {
        printf("error");
    }
    printf("Send: ");
    printf("%s", buff + sizeof(struct udp_header));
    printf("\n");
    free(buff);

    ssize_t bytes_read;
    socklen_t socklen;
    char *payload;
    char RecvBuff[USHRT_MAX];
    memset(RecvBuff, 0, USHRT_MAX);
    int flg = 1;
    while (flg) {
        if ((bytes_read = recvfrom(sock, RecvBuff, USHRT_MAX, 0, (struct sockaddr*) &addr, &socklen)) <= 0) {
            perror("Recv error!");
            close(sock);
            flg = 0;
        }
        struct udp_header* udph = (struct udp_header* ) (RecvBuff + sizeof(struct ip_header));
        if (ntohs(udph->dst) == my_port) {
            payload = (RecvBuff + sizeof(struct ip_header) + sizeof(struct udp_header));
            printf("Recv: ");
            printf("%s", payload);
            printf("\n");
            flg = 0;
        }
    }
}