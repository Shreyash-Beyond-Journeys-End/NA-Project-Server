#include "headers.h"

using namespace std;

// index 0 is not used
const char *table[] = {"", ":method", ":path", ":status", "host", "user-agent",
                       "accept", "content-type", "content-length", "server", "connection"};

void add_header(vector<uint8_t> &out, string name, string value) {
    int idx = 0;
    for (int i = 1; i <= 10; i++)
        if (name == table[i]) idx = i;
    if (idx) {
        out.push_back(0x80 | idx);
    } else {
        out.push_back(0);
        out.push_back(name.size());
        out.insert(out.end(), name.begin(), name.end());
    }
    out.push_back(value.size() >> 8);
    out.push_back(value.size());
    out.insert(out.end(), value.begin(), value.end());
}

bool parse_headers(const vector<uint8_t> &p, map<string, string> &h) {
    size_t pos = 0;
    while (pos < p.size()) {
        uint8_t b = p[pos++];
        string name;
        if (b & 0x80) {
            int idx = b & 0x7f;
            if (idx >= 1 && idx <= 10) name = table[idx];
        } else if (b == 0) {
            if (pos >= p.size()) return false;
            int n = p[pos++];
            if (pos + n > p.size()) return false;
            name = string((const char *)&p[pos], n);
            pos += n;
        } else {
            return false;
        }
        if (pos + 2 > p.size()) return false;
        int vlen = (p[pos] << 8) | p[pos + 1];
        pos += 2;
        if (pos + vlen > p.size()) return false;
        // an index we do not know has no name, the value is skipped
        if (name != "") h[name] = string((const char *)&p[pos], vlen);
        pos += vlen;
    }
    return true;
}
