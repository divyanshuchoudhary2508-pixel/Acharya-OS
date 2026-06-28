/*
 * AcharyaOS - icmp.c
 * ------------------
 * Phase 4, Feature 39: ICMP ping.
 */

#include "icmp.h"
#include "ipv4.h"
#include "kio.h"
#include "kstring.h"

static icmp_stats_t g_stats;

void icmp_init(void) {
    memset(&g_stats, 0, sizeof(g_stats));
}

void icmp_poll(void) {
}

int icmp_ping(uint32_t dst_ip) {
    uint8_t packet[8];
    uint16_t checksum;
    packet[0] = ICMP_TYPE_ECHO_REQUEST;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    packet[4] = 0x12;
    packet[5] = 0x34;
    packet[6] = 0x00;
    packet[7] = 0x01;
    checksum = ipv4_checksum(packet, sizeof(packet));
    packet[2] = (uint8_t)(checksum >> 8);
    packet[3] = (uint8_t)(checksum & 0xFFu);
    if (ipv4_send(dst_ip, 1u, packet, sizeof(packet)) != 0) {
        g_stats.dropped++;
        return -1;
    }
    g_stats.sent++;
    return 0;
}

void icmp_get_stats(icmp_stats_t *stats) { if (stats) { *stats = g_stats; } }
void icmp_reset_stats(void) { memset(&g_stats, 0, sizeof(g_stats)); }

void cmd_ping(int argc, char **argv) {
    uint32_t ip;
    int count = 1;
    if (argc < 2 || !argv || !argv[1]) {
        kprintf("usage: ping <ip> [count]\n");
        return;
    }
    ip = ipv4_from_str(argv[1]);
    if (ip == 0) {
        kprintf("invalid IP address\n");
        return;
    }
    if (argc >= 3 && argv[2]) {
        int value = 0;
        for (size_t i = 0; argv[2][i] >= '0' && argv[2][i] <= '9'; i++) {
            value = (value * 10) + (argv[2][i] - '0');
        }
        if (value > 0) {
            count = value;
        }
    }
    for (int i = 0; i < count; i++) {
        if (icmp_ping(ip) == 0) {
            kprintf("ping %s seq=%d sent\n", argv[1], i + 1);
        } else {
            kprintf("ping %s seq=%d failed\n", argv[1], i + 1);
        }
    }
}
