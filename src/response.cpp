#include <vector>
#include "response.h"
#include "frame.h"
#include "headers.h"

using namespace std;

static const char *status_text(int s) {
    if (s == 200) return "OK";
    if (s == 400) return "Bad Request";
    if (s == 403) return "Forbidden";
    if (s == 404) return "Not Found";
    if (s == 405) return "Method Not Allowed";
    return "Internal Server Error";
}

string content_type(string path) {
    size_t dot = path.rfind('.');
    string ext = dot == string::npos ? "" : path.substr(dot);
    if (ext == ".html") return "text/html";
    if (ext == ".txt") return "text/plain";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "text/javascript";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg") return "image/jpeg";
    return "application/octet-stream";
}

void send_response(int fd, uint32_t stream, int status, string type, string body) {
    vector<uint8_t> h;
    add_header(h, ":status", to_string(status));
    add_header(h, "content-type", type);
    add_header(h, "content-length", to_string(body.size()));
    add_header(h, "server", "bserve/1.0");
    add_header(h, "connection", "keep-alive");
    // an empty body ends the stream on the HEADERS frame
    send_frame(fd, T_HEADERS, body.empty() ? F_END : 0, stream, h.data(), h.size());

    size_t pos = 0;
    while (pos < body.size()) {
        size_t n = body.size() - pos;
        if (n > MAX_PAYLOAD) n = MAX_PAYLOAD;
        bool last = pos + n == body.size();
        send_frame(fd, T_DATA, last ? F_END : 0, stream, (const uint8_t *)body.data() + pos, n);
        pos += n;
    }
}

void send_error(int fd, uint32_t stream, int status) {
    string msg = to_string(status) + " " + status_text(status) + "\n";
    send_response(fd, stream, status, "text/plain", msg);
}
