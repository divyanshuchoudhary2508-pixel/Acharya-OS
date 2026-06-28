/*
 * AcharyaOS - ipv4.h
 * ------------------
 * Phase 4, Feature 38: IPv4.
 */

#ifndef ACHARYAOS_IPV4_H
#define ACHARYAOS_IPV4_H

#include <stddef.h>
#include <stdint.h>

#define IPV4_VERSION        4
#define IPV4_HEADER_MIN     20
#define IPV4_DEFAULT_TTL    64
#define IPV4_MAX_ROUTES     8

typedef void (*ipv4_protocol_handler)(uint32_t src_ip, uint32_t dst_ip,
                                      const uint8_t *payload, size_t payload_len);

typedef struct {
    int valid;
    uint32_t dest;
    uint32_t mask;
    uint32_t gateway;
} ipv4_route_t;

typedef struct {
    uint32_t ip;
    uint32_t gateway;
    uint32_t subnet;
    uint32_t route_count;
    uint32_t send_count;
    uint32_t drop_count;
} ipv4_stats_t;

void ipv4_init(void);
void ipv4_poll(void);
int ipv4_send(uint32_t dst_ip, uint8_t protocol, const void *payload, size_t payload_len);
void ipv4_set_ip(uint32_t ip);
void ipv4_set_gateway(uint32_t gw);
void ipv4_set_subnet(uint32_t subnet);
uint32_t ipv4_get_ip(void);
uint32_t ipv4_get_gateway(void);
uint32_t ipv4_get_subnet(void);
int ipv4_is_local(uint32_t ip);
int ipv4_is_broadcast(uint32_t ip);
int ipv4_is_loopback(uint32_t ip);
uint32_t ipv4_from_str(const char *s);
void ipv4_to_str(uint32_t ip, char *out);
uint16_t ipv4_checksum(const void *data, uint32_t length);
void ipv4_get_stats(ipv4_stats_t *stats);
void ipv4_route_print(void);

#endif /* ACHARYAOS_IPV4_H */
