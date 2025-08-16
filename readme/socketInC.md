# Sockets Overview

## Introduction
Sockets are the apparatus that allows communication between applications, either on the same host or across a network.

Each type of socket has an identifying method:
- **UNIX domain sockets (AF_UNIX)** → Filesystem pathname  
- **IPv4 sockets (AF_INET)** → IPv4 address + port number  
- **IPv6 sockets (AF_INET6)** → IPv6 address + port number  

---

## Types of Sockets

Every sockets implementation provides at least two main types:

### 1. Stream Sockets (`SOCK_STREAM`)
- **Reliable** → Data is either transmitted successfully or fails to transmit.
- **Bidirectional** → Data can be transmitted in both directions (unlike pipes).
    - Example:
      - **Pipe (Shell)**: `ls | wc`
      - **Socket (Network)**: `pr1 || pr2`
- **Byte stream** → No concept of message boundaries (similar to pipes).
- **Connection-oriented** → Each socket connects to one peer.
    - Typical server-client setup:
      ```
      Server listening socket
         │
         ├─ accept() → Client A socket <───> Client A
         └─ accept() → Client B socket <───> Client B
      ```

**Analogy:** Works similarly to the telephone system — connection established first, then continuous communication.

---

### 2. Datagram Sockets (`SOCK_DGRAM`)
- **Message-oriented** → Data is exchanged in discrete messages (datagrams).
- **Unreliable** → Messages may arrive out of order, be duplicated, or never arrive.
- **Connectionless** → No dedicated connection; can send/receive from multiple peers.

---

## Key System Calls

- **`socket()`** → Creates a new socket.
- **`bind()`** → Binds a socket to an address (servers use this to be discoverable).
- **`listen()`** → Prepares a stream socket to accept incoming connections.
- **`accept()`** → Accepts an incoming connection on a listening socket.
- **`connect()`** → Establishes a connection with another socket.
- **`read()` / `write()`** → Transfers data after connection is established.
- **`sendto()` / `recvfrom()`** → Send and receive messages on datagram sockets.

**Note:**  
The `backlog` argument in `listen()` limits the number of pending incoming connections.

---

## Typical Workflows

### Stream Socket (Server)
1. Create socket → `socket()`
2. Bind to address → `bind()`
3. Start listening → `listen()`
4. Accept client connections → `accept()`  
5. Communicate → `read()` / `write()`
6. Close connection → `close()`

### Stream Socket (Client)
1. Create socket → `socket()`
2. Connect to server → `connect()`
3. Communicate → `read()` / `write()`
4. Close connection → `close()`

---

### Datagram Socket (Server)
1. Create socket → `socket()`
2. Bind to address → `bind()`
3. Receive datagrams → `recvfrom()` or `read()`

### Datagram Socket (Client)
1. Create socket → `socket()`
2. Send datagrams → `sendto()` (specifying server address)
3. Optionally use `connect()` to set default peer address, enabling `write()` usage.

---

## Summary

- **Sockets** enable communication between processes over a network or locally.
- A socket’s **communication domain** determines its address format (e.g., UNIX path, IPv4/IPv6 + port).
- Two main types:
    - **Stream sockets** → Reliable, bidirectional, connection-oriented, byte-stream communication.
    - **Datagram sockets** → Unreliable, connectionless, message-oriented communication.
- **Core operations** include creating (`socket()`), binding (`bind()`), listening (`listen()`), connecting (`connect()`), accepting (`accept()`), and sending/receiving data.

