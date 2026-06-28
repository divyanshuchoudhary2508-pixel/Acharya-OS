/*
 * AcharyaOS - ipv4.c
 * ------------------
 * Phase 4, Feature 38: IPv4.
 */

#include "ipv4.h"
#include "arp.h"
#include "eth.h"
#include "kstring.h"
#include "kio.h"

static ipv4_route_t g_routes[IPV4_MAX_ROUTES];
static ipv4_stats_t g_stats;
static uint32_t g_ip = 0xC0A80164u;
static uint32_t g_gateway = 0xC0A80101u;
static uint32_t g_subnet = 0xFFFFFF00u;

static void route_init_defaults(void) {
    memset(g_routes, 0, sizeof(g_routes));
    g_routes[0].valid = 1;
    g_routes[0].dest = g_ip;
    g_routes[0].mask = g_subnet;
    g_routes[0].gateway = 0;
    g_routes[1].valid = 1;
    g_routes[1].dest = 0;
    g_routes[1].mask = 0;
    g_routes[1].gateway = g_gateway;
    g_stats.route_count = 2;
}

static int parse_octet(const char **s, uint32_t *out) {
    uint32_t value = 0;
    int digits = 0;
    while (**s >= '0' && **s <= '9') {
        value = (value * 10u) + (uint32_t)(**s - '0');
        (*s)++;
        digits++;
        if (value > 255u) {
            return 0;
        }
    }
    if (digits == 0) {
        return 0;
    }
    *out = value;
    return 1;
}

void ipv4_init(void) {
    memset(&g_stats, 0, sizeof(g_stats));
    route_init_defaults();
    g_stats.ip = g_ip;
    g_stats.gateway = g_gateway;
    g_stats.subnet = g_subnet;
}

void ipv4_poll(void) {
    arp_poll();
}

void ipv4_set_ip(uint32_t ip) { g_ip = ip; route_init_defaults(); g_stats.ip = g_ip; }
void ipv4_set_gateway(uint32_t gw) { g_gateway = gw; route_init_defaults(); g_stats.gateway = g_gateway; }
void ipv4_set_subnet(uint32_t subnet) { g_subnet = subnet; route_init_defaults(); g_stats.subnet = g_subnet; }
uint32_t ipv4_get_ip(void) { return g_ip; }
uint32_t ipv4_get_gateway(void) { return g_gateway; }
uint32_t ipv4_get_subnet(void) { return g_subnet; }

int ipv4_is_loopback(uint32_t ip) { return (ip >> 24) == 127u; }
int ipv4_is_broadcast(uint32_t ip) { return ip == 0xFFFFFFFFu; }
int ipv4_is_local(uint32_t ip) { return (ip & g_subnet) == (g_ip & g_subnet); }

uint32_t ipv4_from_str(const char *s) {
    uint32_t a, b, c, d;
    if (!s) {
        return 0;
    }
    if (!parse_octet(&s, &a) || *s++ != '.') return 0;
    if (!parse_octet(&s, &b) || *s++ != '.') return 0;
    if (!parse_octet(&s, &c) || *s++ != '.') return 0;
    if (!parse_octet(&s, &d)) return 0;
    if (*s != '\0') return 0;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

void ipv4_to_str(uint32_t ip, char *out) {
    if (!out) return;
    out[0] = '\0';
    kprintf("%u.%u.%u.%u", (unsigned)((ip >> 24) & 0xffu), (unsigned)((ip >> 16) & 0xffu), (unsigned)((ip >> 8) & 0xffu), (unsigned)(ip & 0xffu));
}

uint16_t ipv4_checksum(const void *data, uint32_t length) {
    const uint8_t *bytes = (const uint8_t *) data;
    uint32_t sum = 0;
    while (length > 1) {
        sum += ((uint32_t) bytes[0] << 8) | bytes[1];
        bytes += 2;
        length -= 2;
    }
    if (length > 0) {
        sum += ((uint32_t) bytes[0] << 8);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (uint16_t) (~sum);
}

static uint32_t route_lookup(uint32_t dst_ip) {
    for (size_t i = 0; i < IPV4_MAX_ROUTES; i++) {
        if (!g_routes[i].valid) continue;
        if ((dst_ip & g_routes[i].mask) == (g_routes[i].dest & g_routes[i].mask)) {
            return g_routes[i].gateway ? g_routes[i].gateway : dst_ip;
        }
    }
    return g_gateway ? g_gateway : dst_ip;
}

int ipv4_send(uint32_t dst_ip, uint8_t protocol, const void *payload, size_t payload_len) {
    uint8_t next_hop_mac[ETH_ADDR_LEN];
    uint32_t next_hop_ip;
    if (!payload || payload_len == 0 || payload_len > 1400u) {
        g_stats.drop_count++;
        return -1;
    }
    next_hop_ip = route_lookup(dst_ip);
    if (!arp_resolve(next_hop_ip, next_hop_mac)) {
        g_stats.drop_count++;
        return -1;
    }
    (void) protocol;
    if (eth_send(next_hop_mac, ETH_TYPE_IPV4, payload, payload_len) != 0) {
        g_stats.drop_count++;
        return -1;
    }
    g_stats.send_count++;
    return 0;
}

void ipv4_get_stats(ipv4_stats_t *stats) { if (stats) { *stats = g_stats; } }

void ipv4_route_print(void) {
    kprintf("IPv4 routes:\n");
    for (size_t i = 0; i < IPV4_MAX_ROUTES; i++) {
        if (!g_routes[i].valid) continue;
        kprintf("  %u.%u.%u.%u/%u.%u.%u.%u via %u.%u.%u.%u\n",
                (unsigned)((g_routes[i].dest >> 24) & 0xffu), (unsigned)((g_routes[i].dest >> 16) & 0xffu), (unsigned)((g_routes[i].dest >> 8) & 0xffu), (unsigned)(g_routes[i].dest & 0xffu),
                (unsigned)((g_routes[i].mask >> 24) & 0xffu), (unsigned)((g_routes[i].mask >> 16) & 0xffu), (unsigned)((g_routes[i].mask >> 8) & 0xffu), (unsigned)(g_routes[i].mask & 0xffu),
                (unsigned)((g_routes[i].gateway >> 24) & 0xffu), (unsigned)((g_routes[i].gateway >> 16) & 0xffu), (unsigned)((g_routes[i].gateway >> 8) & 0xffu), (unsigned)(g_routes[i].gateway & 0xffu));
    }
}
