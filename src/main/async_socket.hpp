#pragma once

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace async {

class AsyncSocketException : public std::runtime_error {        
public:
    AsyncSocketException(const std::string msg): std::runtime_error{msg} {}
};

class AsyncSocketData {
private:
    std::string host;
    bool is_host_IPv4_address;
    uint16_t port;
    int max_events;

    // 255.255.255.255
    // 0.0.0.0
    bool is_IPv4_address(std::string& ipv4) 
    {
        char number_list[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        auto isNumber = [&number_list](char character) -> bool {
            for (uint8_t i = 0; i < 10; ++i) {
                if (character == number_list[i])
                    return true;
            }
            return false;
        };

        const uint8_t length = (uint8_t)ipv4.length();
        if (length < 7 || length > 15) {
            return false;
        }

        char byte_number[4] = {};
        int byte_number_index = 0;
        bool is_period = true;
        for (uint8_t i = 0; i < length; ++i) {
            if (isNumber(ipv4[i])) {
                if (is_period) {
                    byte_number_index = 0;
                    is_period = false;
                }
                if (byte_number_index > 2)
                    return false; 
                byte_number[byte_number_index++] = ipv4[i];
                byte_number[byte_number_index] = '\0'; 
            }
            else if (ipv4[i] == '.') {
                if (is_period) 
                    return false;
                int number = atoi(byte_number);
                if (number < 0 || number > 255)
                    return false;
                is_period = true;
            }
            else {
                return false;
            }
        }
        return true;
    }

    int convert_str_to_valid_int(const char* str_to_convert, int valid_min, int valid_max, const char* err_message) 
    {
        errno = 0;
        const long converted_port = strtol(str_to_convert, NULL, 10);
        if (errno == EINVAL || errno == ERANGE || converted_port < valid_min || converted_port > valid_max) {
            throw async::AsyncSocketException(err_message);
        }
        return (int)converted_port;
    }

public:
    AsyncSocketData(const char* _host, const char* str_port, const char* str_max_events)
    :   host{_host}
    {
        is_host_IPv4_address = is_IPv4_address(host);
        port = (uint16_t)convert_str_to_valid_int(str_port, 0, 65535, "Invalid socket port...");
        max_events = convert_str_to_valid_int(str_max_events, 1, __INT_MAX__, "Invalid async max events...");
    }

    ~AsyncSocketData() {}

    bool isHostIPv4Address() {
        return is_host_IPv4_address;
    }

    uint16_t getPort () {
        return port;
    }

    std::string getHost() {
        return host;
    }

    int getMaxEvents() {
        return max_events;
    }
};

} // namespace async