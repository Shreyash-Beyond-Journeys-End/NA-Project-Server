#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include "server.h"
#include "frame.h"
#include "headers.h"
#include "response.h"

using namespace std;

static int handle_request(int fd, uint32_t stream, string root, map<string, string> &h) {
    if (!h.count(":method") || !h.count(":path")) {
        send_error(fd, stream, 400);
        return 400;
    }
    string path = h[":path"];
    if (path.empty() || path[0] != '/') {
        send_error(fd, stream, 400);
        return 400;
    }
    if (h[":method"] != "GET") {
        send_error(fd, stream, 405);
        return 405;
    }
    if (path.find("..") != string::npos) {
        send_error(fd, stream, 403);
        return 403;
    }

    string full = root + path;
    struct stat st;
    if (stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        if (full.back() != '/') full += "/";
        full += "index.html";
    }
    if (stat(full.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        send_error(fd, stream, 404);
        return 404;
    }
    FILE *f = fopen(full.c_str(), "rb");
    if (!f) {
        send_error(fd, stream, 500);
        return 500;
    }
    string body(st.st_size, 0);
    size_t got = body.empty() ? 0 : fread(&body[0], 1, st.st_size, f);
    fclose(f);
    if (got != (size_t)st.st_size) {
        send_error(fd, stream, 500);
        return 500;
    }
    send_response(fd, stream, 200, content_type(full), body);
    return 200;
}

void handle_connection(int fd, string root) {
    while (true) {
        uint8_t h[9];
        if (!read_full(fd, h, 9)) break;
        uint32_t len = (h[0] << 16) | (h[1] << 8) | h[2];
        uint8_t type = h[3];
        uint32_t stream = ((h[5] & 0x7f) << 24) | (h[6] << 16) | (h[7] << 8) | h[8];

        if (len > MAX_PAYLOAD) {
            // we can not find the next frame any more, so answer and close
            send_error(fd, stream, 400);
            printf("bad frame, length %u -> 400, closing\n", len);
            break;
        }
        vector<uint8_t> payload(len);
        if (len > 0 && !read_full(fd, payload.data(), len)) break;

        // unknown types are skipped, the payload is already read
        if (type != T_HEADERS) continue;

        map<string, string> hdr;
        if (!parse_headers(payload, hdr)) {
            send_error(fd, stream, 400);
            printf("malformed header block -> 400\n");
            continue;
        }
        int status = handle_request(fd, stream, root, hdr);
        printf("%s %s -> %d\n", hdr[":method"].c_str(), hdr[":path"].c_str(), status);
    }
    close(fd);
}
