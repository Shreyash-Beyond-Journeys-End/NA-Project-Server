# BHTTP-A, a binary HTTP protocol

Version 1. Written by Shreyash (server) and Pratham (client).

BHTTP-A is a small binary version of HTTP for getting files. A client asks for a path and the server sends
the file back. Everything is sent in frames, a bit like HTTP/2. All numbers are big-endian. This file
is meant to be enough to write a client or a server that works with ours.

## 1. Connection

The client opens one TCP connection and sends its requests on it one after another. The server keeps the
connection open after each answer. The client closes it when it is done.

Every request has a stream id. The client uses odd numbers (1, 3, 5 ...) and the server copies the id of the
request into every frame of the answer.

## 2. Frame header

A frame is a 9 byte header and then the payload.

| Field    | Bits | Meaning |
|----------|------|---------|
| Length   | 24   | size of the payload in bytes, the header is not counted |
| Type     | 8    | kind of frame |
| Flags    | 8    | one bit per option |
| Reserved | 1    | sender sets it to 0, receiver ignores it |
| Stream   | 31   | stream id |

### Why these sizes

HTTP/2 uses 24 / 8 / 8 / 31 and we kept the same widths, because the reasons also fit our protocol.

Length is 24 bits. We only allow 16384 bytes of payload per frame, so 15 bits would do for now. Small frames
mean one big file cannot hold up the connection, and the receiver can use a fixed buffer. We still took 24 bits
so the limit can go up to 16 MB later without changing the header. A frame longer than 16384 is an error.

Type is 8 bits. That is 256 frame types and we use two, so a lot is left for version 2.

Flags is 8 bits, one bit per yes/no option. Only one bit is used now.

Stream is 31 bits plus one reserved bit. The id lets the client match an answer to its request and leaves room
for several requests at the same time in a later version. The extra bit keeps the field at 32 bits, and a later
version can use it.

The whole header is 9 bytes and always the same size, so a receiver reads exactly 9 bytes and knows how
much payload follows.

## 3. Frame types and flags

| Type | Name    | Payload |
|------|---------|---------|
| 0x00 | DATA    | bytes of the file |
| 0x01 | HEADERS | a header block, see section 4 |

| Flag | Name       | Meaning |
|------|------------|---------|
| 0x01 | END_STREAM | last frame of this request or answer |

If a receiver gets a frame type it does not know, it MUST read `Length` bytes, throw them away and go on
with the next frame. It must not close the connection and must not send an error. This is what leaves room
for version 2: new frame types do not break old programs. Flag bits that are not defined are ignored in the
same way.

## 4. Header block

The payload of a HEADERS frame is a list of fields, one after the other until the payload ends. A field is a
name and then a value.

The name starts with one byte:
- top bit set (`1xxxxxxx`): the other 7 bits are an index into the table below
- `0x00`: a literal name follows, first one byte with the length N and then N bytes of name
- any other value: the block is malformed

The value is 2 bytes with the length V and then V bytes.

Table of the ten names that the client and the server really send:

| Index | Name | Index | Name |
|-------|------|-------|------|
| 1 | :method | 6 | accept |
| 2 | :path | 7 | content-type |
| 3 | :status | 8 | content-length |
| 4 | host | 9 | server |
| 5 | user-agent | 10 | connection |

Any other name is sent as a literal. An index the receiver does not know (0 or above 10) is skipped, the value
still has a length so this is easy. If the block ends in the middle of a field it is malformed.

## 5. Request

One HEADERS frame with END_STREAM set. Fields: `:method` (must be `GET`), `:path` (starts with `/`),
and `host`, `user-agent`, `accept`. The server only needs the first two.

## 6. Response

First a HEADERS frame with `:status` (as text, like `200`), `content-type`, `content-length`, `server` and
`connection`. After that come DATA frames with the body, each at most 16384 bytes, and the last one has
END_STREAM. If the body is empty there are no DATA frames and the HEADERS frame itself has END_STREAM.

The server looks for the file under its root folder. A path of `/` or a folder gives the `index.html` in it.

## 7. Errors

An error uses the normal response layout with a short text body such as `404 Not Found`.

| Status | When | Connection |
|--------|------|------------|
| 400 | header block malformed, `:method` or `:path` missing, path does not start with `/` | stays open |
| 400 | `Length` is bigger than 16384 | closed after the answer, the server cannot find the next frame |
| 403 | path contains `..` | stays open |
| 404 | no such file | stays open |
| 405 | method is not GET | stays open |
| 500 | file cannot be read | stays open |

## 8. Client

- opens one connection and sends every request on it
- reads frames until END_STREAM on the stream it asked for
- writes the DATA payloads to stdout in order
- skips frames of unknown types and frames of other streams
- exit code 0 if every status is below 400, 1 if there was a 4xx or 5xx, 2 if the connection or the protocol failed

## 9. Example

`HEXDUMP.md` has a full request and response with every byte explained.

## 10. What version 2 could add

Version 1 only uses two frame types, one flag and ten table entries, so there is room to grow without changing the
9 byte header. Because a receiver MUST skip a frame type it does not know (section 3), a version 2 program can send
new frames to a version 1 program and nothing breaks.

- Several requests at the same time: the stream id is already there, so a v2 client could send requests 1, 3 and 5
  without waiting and the server could interleave their DATA frames.
- A PING frame (for example type 0x02) to check that an idle connection is still alive.
- A GOAWAY frame so the server can say it is shutting down, instead of just closing the socket.
- More table entries (11 to 127) for names that are now sent as literals, like `last-modified` or `cache-control`.
- A bigger frame limit, up to 16 MB, which the 24 bit length already allows.

