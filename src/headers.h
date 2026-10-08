#ifndef HEADERS_H
#define HEADERS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

void add_header(std::vector<uint8_t> &out, std::string name, std::string value);
bool parse_headers(const std::vector<uint8_t> &p, std::map<std::string, std::string> &h);

#endif
