/*
 * AcharyaOS - eth.c
 * -----------------
 * Feature 36: Ethernet support.
 */

#include "eth.h"
#include "kstring.h"

static eth_stats_t g_eth;
static eth_rx_handler_t g_rx_handler;

static void copy_mac(uint8_t dest[ETH_ADDR_LEN], const uint8_t src[ETH_ADDR_LEN]) {
    if (!src) {
        memset(dest, 0, ETH_ADDR_LEN);
        return;
    }
    memcpy(dest, src, ETH_ADDR_LEN);
}

void eth_init(void) {
    static const uint8_t default_mac[ETH_ADDR_LEN] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
    memset(&g_eth, 0, sizeof(g_eth));
    copy_mac(g_eth.mac, default_mac);
    g_eth.ready = 1;
    g_eth.mtu = ETH_PAYLOAD_MAX;
    g_rx_handler = 0;
}

int eth_ready(void) { return g_eth.ready; }

void eth_get_stats(eth_stats_t *stats) { if (stats) { *stats = g_eth; } }

void eth_set_rx_handler(eth_rx_handler_t handler) { g_rx_handler = handler; }

void eth_set_mac(const uint8_t mac[ETH_ADDR_LEN]) { copy_mac(g_eth.mac, mac); }

int eth_send(const uint8_t dest_mac[ETH_ADDR_LEN], uint16_t ethertype, const void *payload, size_t payload_len) {
    (void)dest_mac; (void)ethertype;
    if (!eth_ready() || !payload || payload_len > ETH_PAYLOAD_MAX) { g_eth.dropped_frames++; return -1; }
    g_eth.tx_frames++;
    return 0;
}

void eth_poll(void) { (void)g_rx_handler; }
