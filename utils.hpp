#ifndef BOMBERMAN_UTILS_HPP
#define BOMBERMAN_UTILS_HPP

#include <cstdlib>
#include <cstring>

#include "definitions.hpp"

#define DECIMAL_BASE 10

// Function checks if port passed in *number is valid.
inline bool is_valid_port_number(const char* number) {
    if (number == nullptr) return false;
    if (number[0] == '0' && strlen(number) == 1) return true;
    if (number[0] == '0' || !isdigit(number[0])) return false;
    for (int i = 1; number[i] != 0; i++) {
        if (!isdigit(number[i])) return false;
    }

    char* end;
    errno = 0;
    unsigned long port = strtoul(number, &end, DECIMAL_BASE);
    if (errno != 0 || port > UINT16_MAX) return false;
    return true;
}

// Function returns address and port split after last ':' from addr_port.
address_info get_address_info(const std::string& addr_port) {
    std::string port;
    std::string address;

    size_t found;
    found = addr_port.find_last_of(':');
    address = addr_port.substr(0, found);
    port = addr_port.substr(found + 1);
    if (!is_valid_port_number(port.c_str())) {
        std::cerr << "Invalid port: " << port << '\n';
        exit(EXIT_FAILURE);
    }

    return {address, port};
}

void printDirection(Direction d) {
    switch (d) {
        case Up:
            std::cerr << "up";
            break;
        case Right:
            std::cerr << "right";
            break;
        case Down:
            std::cerr << "down";
            break;
        case Left:
            std::cerr << "left";
            break;
    }
}

#endif  // BOMBERMAN_UTILS_HPP
