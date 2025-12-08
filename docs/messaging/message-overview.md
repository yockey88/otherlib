# Other Environment Messaging

  The goal of the messaging system in Other Environment is "As fluent as talking". What this means is that the messages (their categories, types, data layout, handling, exchange sequences, etc...) should
  feel like a natural language spoken by all parts of the system. An exception to this rule is thread synchronization messages used internally to the `other::thread` interface

  Messages consist of a 1-byte aligned 32 bit header made up of a hex category and ID.

```cpp
#pragma pack(push, 1)
struct message_header {
  uint16_t category;
  uint16_t id;
};
#pragma pack(pop)
```

  Specific message layout is fluid and dependent on context (free-standing reception or defined protocol/exchange sequence?), category/id combination and
  overall system state (udp socket game streams or internal thread control)

## Use Cases

  There are 2 major use cases for messages
  
  1. Thread Messaging
  2. Network Messaging

  For (1) this breaks down into two more categories, one is the private thread API that controls thread synchronization, and the other
  is the main message system for communication between the driver and the network thread.

  For (2) this is the main messaging on any network, whether with TCP or UDP.

## Categories

| Value | Catgegory Name |
| ---- | ---- |
| 0x00 | Notification|
| 0x01 | Acknowledgment |
| 0x02 | Control |
| 0x03 | Command |
| 0x04 | Request  |
| 0x05 | Response  |
| 0x06 | Session Event  |
| 0x07 | Error/Alert |
| ... | ... |

## IDs

| Value | Catgegory Name |
| --- | --- |
| 0x00 | ...|
| 0x01 | Ack |
| 0x02 | Ping |
| 0x03 | Pong |
| 0x04 | Listen For  |
| 0x05 | Connect To  |
| 0x06 | Transmit Message  |
| 0x07 | Check In |
| 0x08 | Session Closed |
| 0x09 | Session Shutdown |
| 0x0a | Session Information |
| 0x0b | Project Cache Information |
| 0x0c | Received Message |
| 0x0d | Shutdown Request |
| ... | ... |
| 0xFFFF | Error/Alert |
