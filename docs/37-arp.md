# Feature 37: ARP

## What it does
ARP maps IPv4 addresses to Ethernet MAC addresses. It sits directly above the Ethernet layer and is required before IPv4 routing can be useful.

## Current behavior
- ARP cache stores IP -> MAC mappings
- static entries can be added
- expired dynamic entries are aged out
- `arp_resolve()` returns a cached MAC or triggers a request
- `arp_print_table()` exposes the table for shell debugging

## Notes
This is the first usable protocol layer above Ethernet. The next feature can build IPv4 routing on top of it.

