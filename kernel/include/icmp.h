/*
 * AcharyaOS - icmp.h
 * ------------------
 * Phase 4, Feature 39: ICMP ping.
 */

#ifndef ACHARYAOS_ICMP_H
#define ACHARYAOS_ICMP_H

#include <stddef.h>
#include <stdint.h>

#define ICMP_TYPE_ECHO_REPLY    0u
#define ICMP_TYPE_ECHO_REQUEST  8u

typedef struct {
    uint32_t sent;
    uint32_t received;
    uint32_t replied;
    uint32_t dropped;
} icmp_stats_t;

void icmp_init(void);
void icmp_poll(void);
int icmp_ping(uint32_t dst_ip);
void icmp_get_stats(icmp_stats_t *stats);
void icmp_reset_stats(void);
void cmd_ping(int argc, char **argv);

#endif /* ACHARYAOS_ICMP_H */
