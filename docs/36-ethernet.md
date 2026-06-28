# Feature 36: Ethernet Support

## What it does
Ethernet is the base layer for the networking roadmap. It exposes a tiny kernel API for sending frames, tracking statistics, and registering a receive handler for future protocol layers.

## Current design
- `eth_init()` sets up Ethernet state
- `eth_send()` records transmission requests
- `eth_poll()` is the hook later layers will call
- `eth_get_stats()` exposes counters for shell/debug output

## What it does not do yet
- No real NIC driver binding yet
- No ARP/IP/TCP logic yet
- No QEMU network device integration yet

This is the foundation layer for the later networking features.
