we use checksum method to just verify if the packets that was sent are the ones that 
received in the destination.

so the main idea is to break the data into n bits then calculate to get Sum, 
once this Sum is gotton then we calcuate the checkSumb by making ~Sum 

then we send that to the destination and it should equal 0xFFFF..FFF [depeneds on n that we choose]
why it should be 0xFFFF..FFF cuz Sum + ~Sum would always equal to 0xFFFF..FFF 


---


setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out))
=> setting time limit for receiving reply from Dest


----
icmp_pkt->icmp_type = ICMP_ECHO;
icmp_pkt->icmp_code = 0;
=> That code sets the ICMP code field to give  more details 
icmp_type = 8 → Echo Request
For Echo Request, the code must always be 0 cuz there is no details about sending that echo request.


icmp_pkt->icmp_id = getpid() & 0xFFFF;
=> icmp_id = getpid() & 0xFFFF → use process ID (masked to 16 bits) so we can identify replies.


icmp_pkt->icmp_seq = seq;
=> icmp_seq = seq → sequence number (incremented for each packet).

---
struct ip* ip_hdr = (struct ip*)recvbuf
hlen = ip_hdr->ip_hl << 2
=> Exactly! ip_hdr->ip_hl gives the number of 32-bit words (chunks) in the IP header. Since each word is 4 bytes, you multiply by 4 (or shift left by 2) to get the total header length in bytes.

---

TTL basically counts how many hops (routers) the packet can still take before being discarded.

---

