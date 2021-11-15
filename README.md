A epoll-based server and thread-per-send/recv client.

# Build
```
cmake .
make
```

# Usage
- To run as server: `ncc <port>`
- To run as client: `ncc <host> <port>`

# Program behaviour
- When run as a client, the program waits for newline-delimited console input, sends it to the server, gets an Ack message back, and shows the roundtrip time and size of original message.
- When run as a server, in addition to accepting clients, it also waits for newline-delimited console input to send to all clients. It then gets Ack from all clients.

# How the server design is arrived at:
- The server needs to read incoming TCP streams: SocketReader
- The incoming TCP streams need to be collected and incrementally formed into discrete objects (aka frames): Deserialiser
- Those objects need to be processed: Generic FrameHandler classes
- The processed objects get transformed further into new objects (Ack messages) to be sent out. These new objects need be enqueued into one big stream and dispatched incrementally into TCP streams: Serialiser
- The server needs to write outgoing TCP streams: SocketWriter
- We need to benchmark message round-trip times: IOBenchmark
- I want an epoll-based implementation for scalability: EpollController

# Client design
The client is an unremarkable classic one-thread-per-io-direction implementation:
1 receiving thread, 1 sending thread, 1 GUI thread.
