#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:UartTool"
#include "esp_lib_utils.h"

#include <cctype>
#include "uart_tool_format.hpp"

namespace esp_brookesia::apps {

std::string uart_tool_bytes_to_hex(const uint8_t *data, size_t len)
{
    static const char digits[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(len * 3);
    for (size_t i = 0; i < len; i++) {
        out.push_back(digits[data[i] >> 4]);
        out.push_back(digits[data[i] & 0x0F]);
        out.push_back(' ');
    }
    return out;
}

bool uart_tool_hex_to_bytes(const char *text, std::vector<uint8_t> &out)
{
    out.clear();
    int nibbles = 0;
    uint8_t value = 0;
    for (const char *p = text; ; p++) {
        char c = *p;
        if (c == '\0' || c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if (nibbles == 1) {
                return false;   // odd number of hex digits in a group
            }
            nibbles = 0;
            if (c == '\0') {
                return true;
            }
            continue;
        }
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return false;       // invalid character
        }
        value = (value << 4) | digit;
        if (++nibbles == 2) {
            out.push_back(value);
            value = 0;
            nibbles = 0;
        }
    }
}

std::string uart_tool_bytes_to_ascii(const uint8_t *data, size_t len)
{
    std::string out;
    out.reserve(len);
    for (size_t i = 0; i < len; i++) {
        char c = static_cast<char>(data[i]);
        if (std::isprint(static_cast<unsigned char>(c)) || c == '\r' || c == '\n' || c == '\t') {
            out.push_back(c);
        } else {
            out.push_back('.');
        }
    }
    return out;
}

} // namespace esp_brookesia::apps
