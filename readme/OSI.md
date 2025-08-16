# Ping and Raw Sockets in C

## Why Ping Uses ICMP
- ICMP is an IP-layer protocol for network diagnostics.  
- Echo Request / Reply = “are you alive?” / “yes, I’m here”.  
- Works even if no TCP/UDP services are running.  

## Raw Sockets
- **SOCK_RAW** allows direct access to the IP layer to build custom packets (e.g., ICMP). give us more control for structuring the packet. 
- **SOCK_STREAM** (TCP) and **SOCK_DGRAM** (UDP) work at the transport layer.


## IP vs Transport Layer
- **IP layer:** moves packets machine → machine (identified by IP).  
- **Transport layer:** moves data to the correct app (identified by port).  

## Data vs Packet Example
- **Data (application):** `"Hello"` <br><br>
- **Transport segment (UDP example):** <br>
`[ Src Port: 12345 | Dst Port: 8080 | Length | Checksum | Data: "Hello" ]`<br><br>
- **IP packet:** <br>
`[ Src IP: 192.168.1.10 | Dst IP: 192.168.1.20 | TTL | Protocol=UDP | Payload: [ UDP Header + "Hello" ] ]`  

## Summary
- Ping uses **SOCK_RAW** + ICMP to check host availability.  
- TCP/UDP sockets cannot send ICMP because ICMP is **not a transport protocol**.
