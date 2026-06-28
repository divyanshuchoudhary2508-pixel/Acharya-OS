# Feature 39: ICMP / Ping

## What it does
ICMP gives the OS a simple way to test network reachability. It is the first visible user-level networking command in the roadmap.

## Current behavior
- `icmp_ping()` sends an echo request via IPv4
- `ping <ip> [count]` is available in the shell
- basic transmit counters are tracked

## Notes
This is a first pass. It creates the ping path before we add full inbound packet handling.
