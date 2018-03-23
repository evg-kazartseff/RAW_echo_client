//
// Created by evgenii on 23.03.18.
//

#include "RAW.h"

uint16_t csum(uint16_t* addr, unsigned short len)
{
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
        *(unsigned char *) (&answer) = *(unsigned char *) w;
        sum += answer;
    }

    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);     /* add carry */
    answer = (uint16_t) ~sum;     /* truncate to 16 bits */
    return (answer);
}

void udp_csum(char* ip_packet)
{
    struct iphdr *iph = (struct iphdr*) ip_packet;
    struct udphdr *udph = (struct udphdr*) (ip_packet + sizeof(struct iphdr));
    unsigned short  udpplen = 0;
    unsigned char* block = NULL;

    struct pseudo_header* ph = NULL;

    udpplen = ntohs(udph->len);

    ph = malloc(sizeof(struct pseudo_header));
    if (ph == NULL) {
        perror("Error: Malloc");
        exit(EXIT_FAILURE);
    }

    ph->source_address = iph->saddr;
    ph->dest_address= iph->daddr;
    ph->placeholder= 0;
    ph->protocol = iph->protocol;
    ph->udp_length= udph->len;

    block = malloc(sizeof(struct pseudo_header) + udpplen);
    if (block == NULL) {
        perror("Error: Malloc");
        exit(EXIT_FAILURE);
    }

    udph->check = 0;

    memcpy(block, ph, sizeof(struct pseudo_header));
    memcpy(block + sizeof(struct pseudo_header), udph, udpplen);
    free(ph);

    udph->check = csum((uint16_t*) block, sizeof(struct pseudo_header) + udpplen);
    free(block);
}