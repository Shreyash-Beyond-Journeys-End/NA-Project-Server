#include <cstring>
#include <vector>
#include <unistd.h>
#include "frame.h"

using namespace std;

bool read_full(int fd, uint8_t *buf, size_t n) {
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, buf + got, n - got);
        if (r <= 0) return false;
        got += r;
    }
    return true;
}

bool write_full(int fd, const uint8_t *buf, size_t n) {
    size_t sent = 0;
    while (sent < n) {
        ssize_t w = write(fd, buf + sent, n - sent);
        if (w <= 0) return false;
        sent += w;
    }
    return true;
}

void send_frame(int fd, uint8_t type, uint8_t flags, uint32_t stream, const uint8_t *data, size_t len) {
    vector<uint8_t> f(9 + len);
    // length is 24 bits, so three bytes
    f[0] = len >> 16;
    f[1] = len >> 8;
    f[2] = len;
    f[3] = type;
    f[4] = flags;
    f[5] = stream >> 24;
    f[6] = stream >> 16;
    f[7] = stream >> 8;
    f[8] = stream;
    if (len) memcpy(&f[9], data, len);
    write_full(fd, f.data(), f.size());
}
