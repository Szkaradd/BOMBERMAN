#include "utils.hpp"
#include <boost/asio.hpp>

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

void modify_message_and_increase_pointer(char *&message, void *src, size_t len) {
    memcpy(message, src, len);
    message += (len * sizeof(uint8_t));
}

void serialize_players_map(char *&pointer, const std::map<player_id_t, Player>& players) {
    auto map_size = htonl((uint32_t) players.size());
    modify_message_and_increase_pointer(pointer, &map_size, sizeof(uint32_t));
    for (const auto& p : players) {
        player_id_t id = p.first;
        modify_message_and_increase_pointer(pointer, &id, sizeof(player_id_t));

        auto p_name_size = (uint8_t) p.second.player_name.length();
        modify_message_and_increase_pointer(pointer, &p_name_size, sizeof(uint8_t));

        modify_message_and_increase_pointer(
                pointer, (void*)p.second.player_name.c_str(),
                p.second.player_name.length() * sizeof(char));

        auto p_addr_size = (uint8_t) p.second.player_address.length();
        modify_message_and_increase_pointer(pointer, &p_addr_size, sizeof(uint8_t));

        modify_message_and_increase_pointer(
                pointer, (void*)p.second.player_address.c_str(),
                p.second.player_address.length() * sizeof(char));
    }
}

void serialize_players_positions(char *&pointer,const std::map<player_id_t, Position>& player_positions) {
    auto map_size = htonl((uint32_t) player_positions.size());
    modify_message_and_increase_pointer(pointer, &map_size, sizeof(uint32_t));
    for (const auto& p : player_positions) {
        player_id_t id = p.first;
        uint16_t x_to_send = htons(p.second.x);
        uint16_t y_to_send = htons(p.second.y);
        modify_message_and_increase_pointer(pointer, &id, sizeof(player_id_t));
        modify_message_and_increase_pointer(pointer, &x_to_send, sizeof(uint16_t));
        modify_message_and_increase_pointer(pointer, &y_to_send, sizeof(uint16_t));
    }
}

void serialize_position(char *&pointer, Position p) {
    uint16_t x_to_send = htons(p.x);
    uint16_t y_to_send = htons(p.y);
    modify_message_and_increase_pointer(pointer, &x_to_send, sizeof(uint16_t));
    modify_message_and_increase_pointer(pointer, &y_to_send, sizeof(uint16_t));
}

void serialize_positions_list(char *&pointer, const std::vector<Position>& positions) {
    auto list_size = htonl((uint32_t) positions.size());
    modify_message_and_increase_pointer(pointer, &list_size, sizeof(uint32_t));
    for (const auto &p : positions) {
        serialize_position(pointer, p);
    }
}

void serialize_bombs_list(char *&pointer, const std::vector<Bomb>& bombs) {
    auto list_size = htonl((uint32_t) bombs.size());
    modify_message_and_increase_pointer(pointer, &list_size, sizeof(uint32_t));
    for (const auto &p : bombs) {
        serialize_position(pointer, p.position);
        uint16_t timer_to_send = htons(p.timer);
        modify_message_and_increase_pointer(pointer, &timer_to_send, sizeof(uint16_t));
    }
}

void serialize_scores_map(char *&pointer, const std::map<player_id_t, score_t>& scores) {
    auto map_size = htonl((uint32_t) scores.size());
    modify_message_and_increase_pointer(pointer, &map_size, sizeof(uint32_t));
    for (const auto &s: scores) {
        player_id_t id = s.first;
        score_t score_to_send = htonl(s.second);
        modify_message_and_increase_pointer(pointer, &id, sizeof(player_id_t));
        modify_message_and_increase_pointer(pointer, &score_to_send, sizeof(score_t));
    }
}

size_t calculate_players_map_size(const std::map<player_id_t, Player>& players) {
    size_t res = sizeof(uint32_t);
    for (const auto &p: players) {
        res += sizeof(p.first);
        res += 2 * sizeof(uint8_t);
        res += p.second.player_name.length() * sizeof(char);
        res += p.second.player_address.length() * sizeof(char);
    }
    return res;
}
