//
// Created by evgenii on 22.01.18.
//

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <string.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <stdlib.h>


unsigned short csum(char* ptr, size_t nbytes) {
    long sum;
    unsigned short oddbyte;
    unsigned short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 1;
    }
    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char*) &oddbyte) = *(u_char*) ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (unsigned short) ~sum;

    return (answer);
}


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
    int on = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

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
    char* ip = "192.168.0.106";
    in_addr_t my_ip;
    if ((my_ip = inet_addr(ip)) == 0) {
        perror("Error: Can't fill ip address");
        close(sock);
        return 1;
    };

    size_t data_len = 11;
    char data[data_len];
    memcpy(data, "Hello world", data_len);

    size_t size_udp_header = sizeof(struct udphdr);
    size_t size_udp_packet = size_udp_header + data_len;
    size_t size_ip_header = sizeof(struct iphdr);
    size_t size_packet = size_ip_header + size_udp_packet;


    char* udp_packet = malloc(size_udp_packet * sizeof(char));
    memset(udp_packet, 0, size_udp_packet);
    struct udphdr* udp = (struct udphdr*) udp_packet;
    udp->source = htons(my_port);
    udp->dest = htons((uint16_t) port);
    udp->len = htons((uint16_t) size_udp_packet);
    udp->check = 0;
    memcpy(udp_packet +  sizeof(struct udphdr), data, data_len);



    char* packet = malloc(size_packet * sizeof(char));
    struct iphdr* ip_header = (struct iphdr*) packet;
    ip_header->version = 4;
    ip_header->ihl = 5;
    ip_header->tos = 0;
    ip_header->tot_len = htons((uint16_t) size_packet);
    ip_header->id = 0;
    ip_header->frag_off = 64;
    ip_header->ttl = 64;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->check = 0;
    ip_header->daddr =  ip_addr.s_addr;
    ip_header->saddr = my_ip;

    memcpy(packet + size_ip_header, udp_packet, size_udp_packet);
    free(udp_packet);

    ip_header->check = (uint16_t) csum(packet, size_packet);

    if (sendto(sock, packet, size_packet, 0, (struct sockaddr*) &addr, sizeof(struct sockaddr_in)) < 0) {
        printf("error");
    }
    printf("Send: ");
    printf("%s", udp_packet + sizeof(struct udphdr));
    printf("\n");
    free(udp_packet);

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
        struct udphdr* udph = (struct udphdr* ) (RecvBuff + sizeof(struct iphdr));
        if (ntohs(udph->dest) == my_port) {
            payload = (RecvBuff + sizeof(struct iphdr) + sizeof(struct udphdr));
            printf("Recv: ");
            printf("%s", payload);
            printf("\n");
            flg = 0;
        }
    }
}