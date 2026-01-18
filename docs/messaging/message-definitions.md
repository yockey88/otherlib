# Message Definitions

## Environment Control

  N/A

## Protocols

 Because of the context dependence of the message layout in Other Environment, message definitions are organized here by `protocol` a term that
 attempts to capture the abstract idea of the entire context in which the message is transmitted/received. For example, the `free-stranding protocol` simply
 means the idle state of the system. For example, the Other Server receives a 'join request' at some random point in time.

 Field abbreviations are defined at the bottom of the page

---

## Protocol List

### Check In Protocol

#### Check In Messages

`CONTROL PING (202)`

```toml
control-ping = {
  SID = integer
}
```

`CONTROL PING (203)`

```toml
control-pong = {
  SID = integer
}
```

`REQUEST SESSION-CHECK-IN (407)`

```toml
request-session-check-in = {
  SID = integer,
}
```

`ACKNOWLEDGMENT ACK (101)`

```toml
acknowledgment-ack = {
  MHDR = message-header,
  ACK = uint8
}
```
  
#### Check In Description

  When the Other Server accepts a connection it waits until the new client sends a `CONTROL PING` message. `SID` in this message
  will be either `-1` (represented by 8 bytes of `0xFF`) or some non-zero 8-byte integer. Below are the following possibilities

  1. The Server has a session ID for the session and the client's `202` contains `-1` (most probable case).
  2. The Server has a session ID for the session and the client's `202` contains a matching ID.
  3. The Server has a session ID for the session and the client's `202` contains a non-matching, valid ID
  4. The Server does not have a session ID for the session and the client's `202` contains `-1`.
  5. [TODO] The Server does not have a session ID for the session and the client's `202` contains a valid ID that does not already exist.
  6. [TODO] The Server does not have a session ID for the session and the client's `202` contains a valid ID that already exists

  In cases 1 and 3 The Server overrides the client's session ID. In case 2, there is nothing to be done, both The Server and Client agree. In case 4 neither The Server nor The Client
  have an ID so The Server generates one and overrides the client's ID. In case 5 The Server adopts The Client's ID. In case 6 the server generates a new ID and overrides The Client's ID.

  In all cases The Server responds with a `CONTROL PONG` message. `SID` in this message must be the `SID` received from the The Client. The Server will then send a `REQUEST SESSION-CHECK-IN` message with
  the final `SID` value that must be adopted from there. The Client must respond with an `ACKNOWLEDGMENT ACK` with the `MHDR  = REQUEST SESSION-CHECK-IN`

---

### Heartbeat Protocol

#### Heartbeat Messages

`CONTROL PING (202)`

```toml
control-ping = {
  SID = integer
}
```

`CONTROL PING (203)`

```toml
control-pong = {
  SID = integer
}
```

#### Heartbeat

  Every `10s` The Server sends a `CONTROL PING` message containing the current `SID` and The Client should respond with a matching `CONTROL PONG` within roughly a second.
  After `TBD` missed heartbeats, The Server will count The Client as 'down' (compared to 'lost').

---

### Session Information Request Protocol

#### Session Information Request Messages

`REQUEST SESSION-INFORMATION (40a)`

```toml
request-session-information = {
}
```

`REQUEST SESSION-INFORMATION (40a)`

```toml
response-session-information = {
}
```

#### Description

  The Server can choose at any time to send a `REQUEST SESSION-INFORMATION` message to The Client. The Client is expected to respond within `10s` with the information indicated
  by the flags in a `RESPONSE SESSION-INFORMATION` message. If The Client is not able to respond within `10s` (either due to missing data or other issues) The Client should send a negative `ACKNOWLEDGEMENT ACK`

## Fields

| Abbreviation | Meaning |
| --- | --- |
| SID | Session ID |
| MHDR | Message Header |
| ACK | Ack/Nack Flag |
|||
