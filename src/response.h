#ifndef RESPONSE_H
#define RESPONSE_H

#include <cstdint>
#include <string>

std::string content_type(std::string path);
void send_response(int fd, uint32_t stream, int status, std::string type, std::string body);
void send_error(int fd, uint32_t stream, int status);

#endif
