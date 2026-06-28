/*
 * AcharyaOS - arp.c
 * -----------------
 * Phase 4, Feature 37: ARP support.
 */

#include "arp.h"
#include "timer.h"
#include "kstring.h"
#include "kio.h"

static arp_entry_t g_cache[ARP_CACHE_MAX];
static arp_stats_t g_stats;
static uint32_t g_local_ip = 0xC0A80164u; /* 192.168.1.100 */
static uint8_t g_local_mac[ETH_ADDR_LEN];

static void mac_copy(uint8_t dest[ETH_ADDR_LEN], const uint8_t src[ETH_ADDR_LEN]) {
    memcpy(dest, src, ETH_ADDR_LEN);
}

static int find_slot(uint32_t ip) {
    for (size_t i = 0; i < ARP_CACHE_MAX; i++) {
        if (g_cache[i].valid && g_cache[i].ip == ip) {
            return (int) i;
        }
    }
    for (size_t i = 0; i < ARP_CACHE_MAX; i++) {
        if (!g_cache[i].valid) {
            return (int) i;
        }
    }
    return 0;
}

static void insert_entry(uint32_t ip, const uint8_t mac[ETH_ADDR_LEN], uint8_t permanent) {
    int slot = find_slot(ip);
    g_cache[slot].ip = ip;
    mac_copy(g_cache[slot].mac, mac);
    g_cache[slot].last_seen = (uint32_t) timer_get_ticks();
    g_cache[slot].valid = 1;
    g_cache[slot].permanent = permanent;
}

void arp_init(void) {
    static const uint8_t broadcast_like_local[ETH_ADDR_LEN] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
    memset(g_cache, 0, sizeof(g_cache));
    memset(&g_stats, 0, sizeof(g_stats));
    mac_copy(g_local_mac, broadcast_like_local);
    eth_set_mac(g_local_mac);
    insert_entry(g_local_ip, g_local_mac, 1);
}

void arp_poll(void) {
    uint32_t now = (uint32_t) timer_get_ticks();
    for (size_t i = 0; i < ARP_CACHE_MAX; i++) {
        if (!g_cache[i].valid || g_cache[i].permanent) {
            continue;
        }
        if ((now - g_cache[i].last_seen) > ARP_TIMEOUT_TICKS) {
            g_cache[i].valid = 0;
        }
    }
}

int arp_resolve(uint32_t ip, uint8_t out_mac[ETH_ADDR_LEN]) {
    for (size_t i = 0; i < ARP_CACHE_MAX; i++) {
        if (g_cache[i].valid && g_cache[i].ip == ip) {
            if (out_mac) {
                mac_copy(out_mac, g_cache[i].mac);
            }
            g_stats.hits++;
            return 1;
        }
    }
    g_stats.misses++;
    arp_send_request(ip);
    return 0;
}

void arp_add_static(uint32_t ip, const uint8_t mac[ETH_ADDR_LEN]) {
    if (!mac) {
        return;
    }
    insert_entry(ip, mac, 1);
}

void arp_send_request(uint32_t target_ip) {
    uint8_t request[28];
    memset(request, 0, sizeof(request));
    request[0] = 0x00; request[1] = 0x01;
    request[2] = 0x08; request[3] = 0x00;
    request[4] = 0x06; request[5] = 0x04;
    request[6] = 0x00; request[7] = 0x01;
    memcpy(&request[8], g_local_mac, 6);
    request[14] = (uint8_t)(g_local_ip >> 24);
    request[15] = (uint8_t)(g_local_ip >> 16);
    request[16] = (uint8_t)(g_local_ip >> 8);
    request[17] = (uint8_t)(g_local_ip);
    request[24] = (uint8_t)(target_ip >> 24);
    request[25] = (uint8_t)(target_ip >> 16);
    request[26] = (uint8_t)(target_ip >> 8);
    request[27] = (uint8_t)(target_ip);
    eth_send((const uint8_t[ETH_ADDR_LEN]){ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, ETH_TYPE_ARP, request, sizeof(request));
    g_stats.requests++;
}

void arp_ip_to_str(uint32_t ip, char *out) {
    if (!out) {
        return;
    }
    out[0] = '\0';
    /* Not used by the shell yet; kept for future formatting helpers. */
    (void) ip;
}

void arp_get_stats(arp_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = g_stats;
    stats->total = ARP_CACHE_MAX;
    stats->valid = 0;
    for (size_t i = 0; i < ARP_CACHE_MAX; i++) {
        if (g_cache[i].valid) {
            stats->valid++;
        }
    }
}

void arp_print_table(void) {
    kprintf("ARP table:\n");
    for (size_t i = 0; i < ARP_CACHE_MAX; i++) {
        if (!g_cache[i].valid) {
            continue;
        }
        kprintf("  %u.%u.%u.%u -> %02x:%02x:%02x:%02x:%02x:%02x%s\n",
                (unsigned)((g_cache[i].ip >> 24) & 0xffu),
                (unsigned)((g_cache[i].ip >> 16) & 0xffu),
                (unsigned)((g_cache[i].ip >> 8) & 0xffu),
                (unsigned)(g_cache[i].ip & 0xffu),
                g_cache[i].mac[0], g_cache[i].mac[1], g_cache[i].mac[2],
                g_cache[i].mac[3], g_cache[i].mac[4], g_cache[i].mac[5],
                g_cache[i].permanent ? " (static)" : "");
    }
}

