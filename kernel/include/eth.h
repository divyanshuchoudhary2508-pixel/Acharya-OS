/*
 * AcharyaOS - eth.h
 * -----------------
 * Phase 4, Feature 36: Ethernet support.
 */

#ifndef ACHARYAOS_ETH_H
#define ACHARYAOS_ETH_H

#include <stddef.h>
#include <stdint.h>

#define ETH_ADDR_LEN 6
#define ETH_PAYLOAD_MAX 1500
#define ETH_FRAME_MAX   1518
#define ETH_TYPE_ARP    0x0806u
#define ETH_TYPE_IPV4   0x0800u

typedef struct {
    int ready;
    uint8_t mac[ETH_ADDR_LEN];
    uint64_t tx_frames;
    uint64_t rx_frames;
    uint64_t dropped_frames;
    uint32_t mtu;
} eth_stats_t;

typedef void (*eth_rx_handler_t)(const uint8_t *src_mac, uint16_t ethertype, const void *payload, size_t payload_len);

void eth_init(void);
int eth_ready(void);
void eth_get_stats(eth_stats_t *stats);
void eth_set_rx_handler(eth_rx_handler_t handler);
int eth_send(const uint8_t dest_mac[ETH_ADDR_LEN], uint16_t ethertype, const void *payload, size_t payload_len);
void eth_poll(void);
void eth_set_mac(const uint8_t mac[ETH_ADDR_LEN]);

#endif /* ACHARYAOS_ETH_H */
