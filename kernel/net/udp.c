/*
 * AcharyaOS - udp.c
 * -----------------
 * Phase 4, Feature 40: UDP.
 */

#include "udp.h"
#include "ipv4.h"
#include "kstring.h"

static udp_socket_t g_sockets[UDP_MAX_SOCKETS];
static udp_stats_t g_stats;
static int g_ready;
static uint16_t g_next_ephemeral = 49152u;

static int find_socket_index(uint16_t local_port) {
    for (size_t i = 0; i < UDP_MAX_SOCKETS; i++) {
        if (g_sockets[i].used && g_sockets[i].local_port == local_port) {
            return (int) i;
        }
    }
    return -1;
}

static int alloc_socket_slot(void) {
    for (size_t i = 0; i < UDP_MAX_SOCKETS; i++) {
        if (!g_sockets[i].used) {
            return (int) i;
        }
    }
    return -1;
}

void udp_register(void) {
    /* Placeholder hook for later protocol dispatch integration. */
}

void udp_init(void) {
    memset(g_sockets, 0, sizeof(g_sockets));
    memset(&g_stats, 0, sizeof(g_stats));
    g_ready = 1;
    udp_register();
}

void udp_poll(void) {
    ipv4_poll();
}

int udp_bind(uint16_t local_port, udp_recv_callback cb) {
    int slot;
    if (!g_ready || local_port == 0) {
        return UDP_NO_SOCKET;
    }
    if (find_socket_index(local_port) >= 0) {
        return UDP_NO_SOCKET;
    }
    slot = alloc_socket_slot();
    if (slot < 0) {
        return UDP_NO_SOCKET;
    }
    g_sockets[slot].used = 1;
    g_sockets[slot].local_port = local_port;
    g_sockets[slot].callback = cb;
    g_stats.bound_sockets++;
    return slot;
}

int udp_auto_bind(udp_recv_callback cb) {
    for (uint16_t port = g_next_ephemeral; port != 0; port++) {
        int socket_id = udp_bind(port, cb);
        if (socket_id >= 0) {
            g_next_ephemeral = (port == 65535u) ? 49152u : (uint16_t)(port + 1u);
            return socket_id;
        }
        if (port == 65535u) {
            break;
        }
    }
    return UDP_NO_SOCKET;
}

void udp_unbind(int socket_id) {
    if (socket_id < 0 || socket_id >= UDP_MAX_SOCKETS) {
        return;
    }
    if (g_sockets[socket_id].used) {
        memset(&g_sockets[socket_id], 0, sizeof(g_sockets[socket_id]));
    }
}

static uint16_t udp_checksum(const void *data, size_t length) {
    return ipv4_checksum(data, (uint32_t) length);
}

int udp_send(int socket_id, uint32_t dst_ip, uint16_t dst_port,
             const void *data, size_t length) {
    uint8_t packet[sizeof(uint16_t) * 4 + UDP_MAX_PAYLOAD];
    uint16_t src_port;
    uint16_t udp_len;
    uint16_t checksum;

    if (socket_id < 0 || socket_id >= UDP_MAX_SOCKETS || !g_sockets[socket_id].used) {
        g_stats.dropped_packets++;
        return -1;
    }
    if (!data || length > UDP_MAX_PAYLOAD) {
        g_stats.dropped_packets++;
        return -1;
    }

    src_port = g_sockets[socket_id].local_port;
    udp_len = (uint16_t)(8u + length);
    packet[0] = (uint8_t)(src_port >> 8);
    packet[1] = (uint8_t)(src_port & 0xffu);
    packet[2] = (uint8_t)(dst_port >> 8);
    packet[3] = (uint8_t)(dst_port & 0xffu);
    packet[4] = (uint8_t)(udp_len >> 8);
    packet[5] = (uint8_t)(udp_len & 0xffu);
    packet[6] = 0;
    packet[7] = 0;
    memcpy(packet + 8, data, length);
    checksum = udp_checksum(packet, (size_t) udp_len);
    packet[6] = (uint8_t)(checksum >> 8);
    packet[7] = (uint8_t)(checksum & 0xffu);

    if (ipv4_send(dst_ip, IP_PROTO_UDP, packet, udp_len) != 0) {
        g_stats.dropped_packets++;
        return -1;
    }
    g_stats.tx_packets++;
    return 0;
}

void udp_get_stats(udp_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = g_stats;
}

