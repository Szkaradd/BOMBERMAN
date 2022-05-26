/* This file contains defines of all classes,
 * enums and structs based on task contests. */

#ifndef BOMBERMAN_DEFINITIONS_HPP
#define BOMBERMAN_DEFINITIONS_HPP

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#define DECIMAL_BASE 10

using player_id_t = uint8_t;
using bomb_id_t = uint32_t;
using score_t = uint32_t;

/* Enums from task contents. */

enum MessageToGuiEnum : uint8_t {
    Lobby = 0,
    Game = 1
};

enum GuiInputEnum : uint8_t {
    PlaceBombGui = 0,
    PlaceBlockGui = 1,
    MoveGui = 2
};

enum Direction : uint8_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

enum ClientMessageEnum : uint8_t {
    Join = 0,
    PlaceBomb = 1,
    PlaceBlock = 2,
    Move = 3
};

enum ServerMessageEnum : uint8_t {
    Hello = 0,
    AcceptedPlayer = 1,
    GameStarted = 2,
    Turn = 3,
    GameEnded = 4
};

enum EventType : uint8_t {
    BombPlaced = 0,
    BombExploded = 1,
    PlayerMoved = 2,
    BlockPlaced = 3
};

// Struct storing info about host address and port as strings.
struct address_info {
    std::string address;
    std::string port;

    address_info(std::string sa, std::string p)
            : address(std::move(sa)), port(std::move(p)) {};
    address_info() = default;
};

// Structs from task contents.

struct Player {
    std::string player_name;
    std::string player_address;

    Player(std::string pn, std::string pa) :
            player_name(std::move(pn)), player_address(std::move(pa)) {};
    Player() = default;
};

struct Position {
    uint16_t x{};
    uint16_t y{};

    Position(uint16_t cx, uint16_t cy) : x(cx), y(cy) {};
    Position() = default;
    bool operator<(const Position &that) const {
        if (x < that.x) return true;
        else if (x == that.x) return y < that.y;
        else return false;
    };
};

struct Bomb {
    Position position;
    uint16_t timer{};

    Bomb(Position p, uint16_t t) : position(p), timer(t) {};
    Bomb() = default;
};

class MessageToGui {
public:
    MessageToGui() = default;
    MessageToGuiEnum msg_type{};

    std::string server_name;
    uint8_t player_count{};
    uint16_t size_x{};
    uint16_t size_y{};
    uint16_t game_length{};
    uint16_t explosion_radius{};
    uint16_t bomb_timer{};
    uint16_t turn{};
    std::map<player_id_t , Player> players;
    std::map<player_id_t , Position> player_positions;
    std::set<Position> blocks;
    std::map<bomb_id_t, Bomb> bombs;
    std::set<Position> explosions;
    std::map<player_id_t , score_t> scores;
};

class Event {
public:
    EventType event_type{};
    bomb_id_t bomb_id{};
    player_id_t player_id{};
    Position position;
    std::set<player_id_t> robots_destroyed;
    std::set<Position> blocks_destroyed;
};

class GuiInputMessage {
public:
    GuiInputEnum msg_type{};
    Direction direction{};
    GuiInputMessage() = default;
};

class ClientMessage {
public:
    ClientMessageEnum msg_type{};
    std::string player_name;
    Direction direction{};

    ClientMessage() = default;
};

class ServerMessage {
public:
    ServerMessageEnum msg_type{};

    std::string server_name;
    uint8_t player_count{};
    uint16_t size_x{};
    uint16_t size_y{};
    uint16_t game_length{};
    uint16_t explosion_radius{};
    uint16_t bomb_timer{};
    uint16_t turn{};
    player_id_t player_id{};
    Player player;
    std::map<player_id_t , Player> players;
    std::map<player_id_t , score_t> scores;
    std::vector<Event> events;
};

#endif //BOMBERMAN_DEFINITIONS_HPP
