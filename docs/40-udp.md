# Feature 40: UDP

## What it does
UDP is the simple datagram layer above IPv4. It gives the OS a lightweight send path for future DNS and other small network services.

## Current behavior
- socket slots can be bound to local ports
- `udp_send()` builds a UDP datagram and hands it to IPv4
- basic counters track sent and dropped packets

## Notes
This first pass focuses on the transmit side and socket bookkeeping. It sets the groundwork for DNS and later application protocols.

