# Annotated hexdump, GET /index.html

Made with `./bserve ./www 9000` and `./bcurl -v localhost:9000/index.html`.
The file is 176 bytes long.

## Request (client to server), one frame, 9 + 55 = 64 bytes

```
00 00 37 01 01 00 00 00 01      frame header
81 00 03 47 45 54               field
82 00 0b 2f 69 6e 64 65 78 2e 68 74 6d 6c   field
84 00 0e 6c 6f 63 61 6c 68 6f 73 74 3a 39 30 30 30   field
85 00 09 62 63 75 72 6c 2f 31 2e 30   field
86 00 03 2a 2f 2a              field
```

| Bytes | Meaning |
|-------|---------|
| `00 00 37` | Length = 0x37 = 55 payload bytes |
| `01` | Type = HEADERS |
| `01` | Flags = END_STREAM (the request has nothing after it) |
| `00 00 00 01` | Reserved bit 0, stream id 1 |
| `81` | indexed name, index 1 = `:method` |
| `00 03` `47 45 54` | value length 3, "GET" |
| `82` | index 2 = `:path` |
| `00 0b` `2f 69 6e 64 65 78 2e 68 74 6d 6c` | length 11, "/index.html" |
| `84` | index 4 = `host` |
| `00 0e` `6c 6f ... 30` | length 14, "localhost:9000" |
| `85` | index 5 = `user-agent` |
| `00 09` `62 63 75 72 6c 2f 31 2e 30` | length 9, "bcurl/1.0" |
| `86` | index 6 = `accept` |
| `00 03` `2a 2f 2a` | length 3, "*/*" |

## Response frame 1: HEADERS, 9 + 50 = 59 bytes

```
00 00 32 01 00 00 00 00 01
83 00 03 32 30 30
87 00 09 74 65 78 74 2f 68 74 6d 6c
88 00 03 31 37 36
89 00 0a 62 73 65 72 76 65 2f 31 2e 30
8a 00 0a 6b 65 65 70 2d 61 6c 69 76 65
```

| Bytes | Meaning |
|-------|---------|
| `00 00 32` | Length = 50 |
| `01` | Type = HEADERS |
| `00` | Flags = none, DATA frames follow |
| `00 00 00 01` | stream 1, same as the request |
| `83` `00 03` "200" | `:status` = 200 |
| `87` `00 09` "text/html" | `content-type` |
| `88` `00 03` "176" | `content-length` = 176 |
| `89` `00 0a` "bserve/1.0" | `server` |
| `8a` `00 0a` "keep-alive" | `connection` |

## Response frame 2: DATA, 9 + 176 = 185 bytes

```
00 00 b0 00 01 00 00 00 01
3c 68 74 6d 6c 3e 0a 3c 68 65 61 64 3e ...   (176 bytes of the file, "<html>\n<head>...")
```

| Bytes | Meaning |
|-------|---------|
| `00 00 b0` | Length = 0xb0 = 176 |
| `00` | Type = DATA |
| `01` | Flags = END_STREAM, this ends the response |
| `00 00 00 01` | stream 1 |
| `3c 68 74 6d 6c 3e ...` | the file, written to stdout by bcurl |

The connection stays open after this. The next request would use stream id 3.
