/*
 * AcharyaOS - arp.h
 * -----------------
 * Phase 4, Feature 37: ARP support.
 */

#ifndef ACHARYAOS_ARP_H
#define ACHARYAOS_ARP_H

#include <stddef.h>
#include <stdint.h>
#include "eth.h"

#define ARP_CACHE_MAX 32
#define ARP_PENDING_MAX 8
#define ARP_TIMEOUT_TICKS 300u
#define ARP_REQUEST_RETRY_TICKS 30u

typedef struct {
    uint32_t ip;
    uint8_t mac[ETH_ADDR_LEN];
    uint32_t last_seen;
    uint8_t valid;
    uint8_t permanent;
} arp_entry_t;

typedef struct {
    uint32_t total;
    uint32_t valid;
    uint32_t hits;
    uint32_t misses;
    uint32_t requests;
    uint32_t replies;
} arp_stats_t;

void arp_init(void);
void arp_poll(void);
int arp_resolve(uint32_t ip, uint8_t out_mac[ETH_ADDR_LEN]);
void arp_add_static(uint32_t ip, const uint8_t mac[ETH_ADDR_LEN]);
void arp_send_request(uint32_t target_ip);
void arp_ip_to_str(uint32_t ip, char *out);
void arp_get_stats(arp_stats_t *stats);
void arp_print_table(void);

#endif /* ACHARYAOS_ARP_H */
