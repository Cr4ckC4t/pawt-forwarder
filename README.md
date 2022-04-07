# Pawt Forwarder

A very basic port forwarder for written in C.

## Usage

**Set the target:**
```c
#define SERV_PORT <port> // Set the server port
#define TARGET_PORT <port> // Set the target port where connections will be forwarded to
#define TARGET_IP <ip string> // Set the target address where connections will be forwarded to
```

**Compile with:**
```bash
gcc pawt-forwarder.c -Wall
```

**Connect to server:**

All incoming data on the server will be fowarded directly to the target. Vice versa, all data originating from the target port will be sent back via the server. The server detects a normal EOF and closes the corresponding sockets but clients may fail to receive an error when a connection (attempt) failed. The server could easily be extended by more explicit behavior in failure cases should someone need it.
