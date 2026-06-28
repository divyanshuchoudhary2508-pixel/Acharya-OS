/*
 * AcharyaOS - udp.h
 * -----------------
 * Phase 4, Feature 40: UDP.
 */

#ifndef ACHARYAOS_UDP_H
#define ACHARYAOS_UDP_H

#include <stddef.h>
#include <stdint.h>

#define IP_PROTO_UDP       17u
#define UDP_MAX_SOCKETS    8
#define UDP_MAX_PAYLOAD    1472u
#define UDP_NO_SOCKET      (-1)

typedef void (*udp_recv_callback)(uint32_t src_ip, uint16_t src_port,
                                   const uint8_t *data, size_t data_len);

typedef struct {
    int used;
    uint16_t local_port;
    udp_recv_callback callback;
} udp_socket_t;

typedef struct {
    uint32_t bound_sockets;
    uint32_t tx_packets;
    uint32_t rx_packets;
    uint32_t dropped_packets;
} udp_stats_t;

void udp_init(void);
void udp_poll(void);
int udp_bind(uint16_t local_port, udp_recv_callback cb);
int udp_auto_bind(udp_recv_callback cb);
void udp_unbind(int socket_id);
int udp_send(int socket_id, uint32_t dst_ip, uint16_t dst_port,
             const void *data, size_t length);
void udp_get_stats(udp_stats_t *stats);
void udp_register(void);

#endif /* ACHARYAOS_UDP_H */

