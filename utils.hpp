#ifndef BOMBERMAN_UTILS_HPP
#define BOMBERMAN_UTILS_HPP

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>

#define DECIMAL_BASE 10

using player_id_t = uint8_t;
using score_t = uint32_t;

// Function checks if port passed in *number is valid.
inline bool is_valid_port_number(const char* number) {
    if (number == nullptr) return false;
    if (number[0] == '0' || !isdigit(number[0]))
        return false;
    for (int i = 1; number[i] != 0; i++) {
        if (!isdigit(number[i]))
            return false;
    }

    char *end;
    errno = 0;
    unsigned long port = strtoul(number, &end, DECIMAL_BASE);
    if (errno != 0 || port > UINT16_MAX)
        return false;
    return true;
}

inline uint16_t read_port(const char *string) {
    if (!is_valid_port_number(string)) {
        std::cerr << "Invalid port: " << string << '\n';
        exit(EXIT_FAILURE);
    }
    unsigned long port = strtoul(string, nullptr, DECIMAL_BASE);
    return (uint16_t) port;
}

struct address_info {
    std::string address;
    std::string port;

    address_info(std::string sa, std::string p)
            : address(std::move(sa)), port(std::move(p)) {};
};

struct Player {
    std::string player_name;
    std::string player_address;

    Player(std::string pn, std::string pa) :
            player_name(std::move(pn)), player_address(std::move(pa)) {};
};

struct Position {
    uint16_t x;
    uint16_t y;

    Position(uint16_t cx, uint16_t cy) : x(cx), y(cy) {};
};

struct Bomb {
    Position position;
    uint16_t timer;

    Bomb(Position p, uint16_t t) : position(p), timer(t) {};
};

void modify_message_and_increase_pointer(char *&message, void *src, size_t len);

address_info get_address_info(const std::string& addr_port);

void serialize_players_map(char *&pointer, const std::map<player_id_t, Player>& players);

void serialize_players_positions(char *&pointer,const std::map<player_id_t, Position>& player_positions);

void serialize_positions_list(char *&pointer, const std::vector<Position>& positions);

void serialize_bombs_list(char *&pointer, const std::vector<Bomb>& bombs);

void serialize_scores_map(char *&pointer, const std::map<player_id_t, score_t>& scores);

size_t calculate_players_map_size(const std::map<player_id_t, Player>& players);

inline size_t calculate_players_position_map_size(const std::map<player_id_t, Position>& player_positions) {
    return sizeof(uint32_t) + player_positions.size() * (sizeof(Position) + sizeof(player_id_t));
}

inline size_t calculate_positions_list_size(const std::vector<Position>& positions) {
    return sizeof(uint32_t) + positions.size() * sizeof(Position);
}

inline size_t calculate_bombs_list_size(const std::vector<Bomb>& bombs) {
    return sizeof(uint32_t) + bombs.size() * (sizeof(Position) + sizeof(uint16_t));
}

inline size_t calculate_scores_map_size(const std::map<player_id_t, score_t>& scores) {
    return sizeof(uint32_t) + scores.size() * (sizeof(score_t) + sizeof(player_id_t));
}

#endif //BOMBERMAN_UTILS_HPP
