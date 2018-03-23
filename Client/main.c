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
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <time.h>
#include "RAW.h"


int main(int argc, char** argv)
{
    if (argc != 3) {
        perror("Use TCP_Client \"IP address\" \"port\"");
        return EXIT_FAILURE;
    }

    char* ifname = "enp7s0";
    uint8_t dst_mac[ETH_ALEN] = {0x00, 0x22, 0x15, 0x9a, 0xf9, 0xd4};
    uint16_t my_port = 5456;
    char* if_ip = "172.16.0.11";

    int sock;
    if ((sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("Error: Can't create socket");
        return EXIT_FAILURE;
    }

    int if_index;
    uint8_t if_addr[ETH_ALEN];
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, strlen(ifname));
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror ("Error: SIOCGIFINDEX");
        close(sock);
        return EXIT_FAILURE;
    }
    if_index = ifr.ifr_ifindex;
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        perror ("Error: SIOCGIFHWADDR");
        close(sock);
        return EXIT_FAILURE;
    }
    memcpy(if_addr, ifr.ifr_ifru.ifru_hwaddr.sa_data, ETH_ALEN);

    int on = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(struct sockaddr_ll));
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = if_index;
    addr.sll_halen = ETH_ALEN;
    addr.sll_pkttype = PACKET_OUTGOING;
    memcpy(addr.sll_addr, dst_mac, ETH_ALEN);
    addr.sll_protocol = htons(ETH_P_IP);

    struct in_addr dst_ip_addr;
    memset(&dst_ip_addr, 0, sizeof(struct in_addr));
    if (inet_aton(argv[1], &dst_ip_addr) == 0) {
        perror("Error: Can't fill if_ip address");
        close(sock);
        return EXIT_FAILURE;
    };
    long dst_port = strtol(argv[2], NULL, 10);
    if (dst_port < 0) {
        perror("Error: Port can't be negative");
        close(sock);
        return EXIT_FAILURE;
    } else if (dst_port > USHRT_MAX) {
        perror("Error: Port can't be more than 65535");
        close(sock);
        return EXIT_FAILURE;
    }

    in_addr_t my_ip;
    if ((my_ip = inet_addr(if_ip)) == 0) {
        perror("Error: Can't fill if_ip address");
        close(sock);
        return EXIT_FAILURE;
    };

    size_t data_len = 11;
    char data[data_len];
    memcpy(data, "Hello world", data_len);

    size_t size_udp_header = sizeof(struct udphdr);
    size_t size_udp_packet = size_udp_header + data_len;
    size_t size_ip_header = sizeof(struct iphdr);
    size_t size_ip_packet = size_ip_header + size_udp_packet;
    size_t size_eth_header = sizeof(struct ether_header);
    size_t size_packet = size_eth_header + size_ip_packet;


    char* udp_packet = malloc(size_udp_packet * sizeof(char));
    memset(udp_packet, 0, size_udp_packet);
    struct udphdr* udp = (struct udphdr*) udp_packet;
    udp->source = htons(my_port);
    udp->dest = htons((uint16_t) dst_port);
    udp->len = htons((uint16_t) size_udp_packet);
    udp->check = 0;
    memcpy(udp_packet + size_udp_header, data, data_len);


    char* ip_packet = malloc(size_ip_packet * sizeof(char));
    struct iphdr* ip_header = (struct iphdr*) ip_packet;
    ip_header->version = 4;
    ip_header->ihl = 5;
    ip_header->tos = 0;
    ip_header->tot_len = htons((uint16_t) size_ip_packet);
    srand((unsigned int) time(NULL));
    ip_header->id = htons((uint16_t) rand());
    ip_header->frag_off = 64;
    ip_header->ttl = 64;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->check = 0;
    ip_header->daddr = dst_ip_addr.s_addr;
    ip_header->saddr = my_ip;
    memcpy(ip_packet + size_ip_header, udp_packet, size_udp_packet);
    free(udp_packet);
    ip_header->check = csum((uint16_t*) ip_packet, (unsigned short) size_ip_header);
    udp_csum(ip_packet);


    char* packet = malloc(size_packet * sizeof(char));
    struct ether_header* eth_header = (struct ether_header*) packet;
    memcpy(eth_header->ether_shost, if_addr, ETH_ALEN);
    memcpy(eth_header->ether_dhost, dst_mac, ETH_ALEN);
    eth_header->ether_type = htons(ETH_P_IP);


    memcpy(packet + size_eth_header, ip_packet, size_ip_packet);
    free(ip_packet);


    if (sendto(sock, packet, size_packet, 0, (struct sockaddr*) &addr, sizeof(struct sockaddr_ll)) < 0) {
        printf("error ");
    }
    printf("Send: ");
    printf("%s", packet + size_eth_header + size_ip_header + size_udp_header);
    printf("\n");
    free(packet);

    struct sockaddr_in recv_addrin;
    ssize_t bytes_read;
    socklen_t socklen;
    char *payload;
    char RecvBuff[USHRT_MAX];
    memset(RecvBuff, 0, USHRT_MAX);
    int flg = 1;
    while (flg) {
        if ((bytes_read = recvfrom(sock, RecvBuff, USHRT_MAX, 0, (struct sockaddr*) &recv_addrin, &socklen)) <= 0) {
            perror("Recv error!");
            close(sock);
            flg = 0;
        }
        struct udphdr* udph = (struct udphdr* ) (RecvBuff + size_eth_header + size_ip_header);
        if (ntohs(udph->dest) == my_port) {
            payload = RecvBuff + size_eth_header + size_ip_header + size_udp_header;
            printf("Recv: ");
            printf("%s", payload);
            printf("\n");
            flg = 0;
        }
    }
}