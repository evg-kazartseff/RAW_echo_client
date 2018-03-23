//
// Created by evgenii on 23.03.18.
//

#ifndef ECHO_RAW_H
#define ECHO_RAW_H


#include <stdint.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

struct pseudo_header
{
    in_addr_t source_address;
    in_addr_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

uint16_t csum(uint16_t* addr, unsigned short len);
void udp_csum(char* ip_packet);

#endif //ECHO_RAW_H
