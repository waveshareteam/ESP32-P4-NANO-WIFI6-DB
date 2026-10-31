#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace esp_brookesia::apps {

void uart_tool_bytes_to_hex(const uint8_t *data, size_t len, std::string &out);
bool uart_tool_hex_to_bytes(const char *text, std::vector<uint8_t> &out);
void uart_tool_bytes_to_ascii(const uint8_t *data, size_t len, std::string &out);

} // namespace esp_brookesia::apps
