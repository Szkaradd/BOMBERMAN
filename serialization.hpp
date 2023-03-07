#ifndef BOMBERMAN_SERIALIZATION_HPP
#define BOMBERMAN_SERIALIZATION_HPP
#include <map>
#include <set>
#include <string>
#include <utility>

#include "buffer.hpp"
#include "definitions.hpp"

/*                                                   *
 * Operators for reading and writing standard types. *
 *                                                   */

// Writing uint8_t operator.
Buffer &operator<<(Buffer &buffer, const uint8_t &val) {
    buffer.writeUint8(val);
    return buffer;
}

// Reading uint8_t operator.
Buffer &operator>>(Buffer &buffer, uint8_t &val) {
    val = buffer.readUint8();
    return buffer;
}

// Writing uint16_t operator.
Buffer &operator<<(Buffer &buffer, const uint16_t &val) {
    buffer.writeUint16(val);
    return buffer;
}

// Reading uint16_t operator.
Buffer &operator>>(Buffer &buffer, uint16_t &val) {
    val = buffer.readUint16();
    return buffer;
}

// Writing uint32_t operator.
Buffer &operator<<(Buffer &buffer, const uint32_t &val) {
    buffer.writeUint32(val);
    return buffer;
}

// Reading uint32_t operator.
Buffer &operator>>(Buffer &buffer, uint32_t &val) {
    val = buffer.readUint32();
    return buffer;
}

// Writing string operator.
Buffer &operator<<(Buffer &buffer, const std::string &str) {
    buffer.writeUint8((uint8_t)str.size());
    buffer.writeString(str);
    return buffer;
}

// Reading string operator.
Buffer &operator>>(Buffer &buffer, std::string &str) {
    str = buffer.readString(buffer.readUint8());
    return buffer;
}

/*                                                    *
 * Operators for reading and writing defined structs and classes. *
 *                                                    */

// Writing struct player operator.
Buffer &operator<<(Buffer &buffer, const Player &player) {
    buffer << player.player_name << player.player_address;
    return buffer;
}

// Reading struct player operator.
Buffer &operator>>(Buffer &buffer, Player &player) {
    buffer >> player.player_name >> player.player_address;
    return buffer;
}

// Writing struct position operator.
Buffer &operator<<(Buffer &buffer, const Position &position) {
    buffer << position.x << position.y;
    return buffer;
}

// Reading struct position operator.
Buffer &operator>>(Buffer &buffer, Position &position) {
    buffer >> position.x >> position.y;
    return buffer;
}

// Writing struct bomb operator.
Buffer &operator<<(Buffer &buffer, const Bomb &bomb) {
    buffer << bomb.position << bomb.timer;
    return buffer;
}

// Reading struct bomb operator.
Buffer &operator>>(Buffer &buffer, Bomb &bomb) {
    buffer >> bomb.position >> bomb.timer;
    return buffer;
}

// Direction read operator.
Buffer &operator>>(Buffer &buffer, Direction &direction) {
    uint8_t dir;
    buffer >> dir;
    if (dir > 3) {
        throw std::invalid_argument("Wrong direction received");
    }
    direction = (Direction)dir;
    return buffer;
}

// Direction write operator.
Buffer &operator<<(Buffer &buffer, const Direction &direction) {
    buffer << (uint8_t)direction;
    return buffer;
}

/*                                                                              *
 * Operators for reading and writing maps sets and vectors of defined objects.  *
 *                                                                              */

// Writing players map operator.
Buffer &operator<<(Buffer &buffer, const std::map<player_id_t, Player> &players) {
    buffer << (uint32_t)players.size();
    for (const auto &elem : players) {
        buffer << elem.first << elem.second;
    }
    return buffer;
}

// Reading players map operator.
Buffer &operator>>(Buffer &buffer, std::map<player_id_t, Player> &players) {
    size_t size = buffer.readUint32();
    players.clear();
    for (size_t i = 0; i < size; i++) {
        player_id_t id;
        Player player;
        buffer >> id >> player;
        players.insert({id, player});
    }
    return buffer;
}

// Writing bombs map operator.
Buffer &operator<<(Buffer &buffer, const std::map<bomb_id_t, Bomb> &bombs) {
    buffer << (uint32_t)bombs.size();
    for (const auto &elem : bombs) {
        // We don't want to send bomb id.
        buffer << elem.second;
    }
    return buffer;
}

// Writing positions map operator.
Buffer &operator<<(Buffer &buffer, const std::map<player_id_t, Position> &positions) {
    buffer << (uint32_t)positions.size();
    for (const auto &elem : positions) {
        buffer << elem.first << elem.second;
    }
    return buffer;
}

// Writing scores map operator.
Buffer &operator<<(Buffer &buffer, const std::map<player_id_t, score_t> &scores) {
    buffer << (uint32_t)scores.size();
    for (const auto &elem : scores) {
        buffer << elem.first << elem.second;
    }
    return buffer;
}

// Reading scores map operator;
Buffer &operator>>(Buffer &buffer, std::map<player_id_t, score_t> &scores) {
    size_t size = buffer.readUint32();
    scores.clear();
    for (size_t i = 0; i < size; i++) {
        player_id_t id;
        score_t score;
        buffer >> id >> score;
        scores.insert({id, score});
    }
    return buffer;
}

// Writing positions set operator.
Buffer &operator<<(Buffer &buffer, const std::set<Position> &positions) {
    buffer << (uint32_t)positions.size();
    for (const auto &elem : positions) {
        buffer << elem;
    }
    return buffer;
}

// Reading positions set operator.
Buffer &operator>>(Buffer &buffer, std::set<Position> &positions) {
    size_t size = buffer.readUint32();
    positions.clear();
    for (size_t i = 0; i < size; i++) {
        Position p;
        buffer >> p;
        positions.insert(p);
    }
    return buffer;
}

// Reading player id's set operator.
Buffer &operator>>(Buffer &buffer, std::set<player_id_t> &players) {
    size_t size = buffer.readUint32();
    players.clear();
    for (size_t i = 0; i < size; i++) {
        player_id_t id;
        buffer >> id;
        players.insert(id);
    }
    return buffer;
}

// Writing player id's set operator.
Buffer &operator<<(Buffer &buffer, const std::set<player_id_t> &players) {
    buffer << (uint32_t)(players.size());
    for (auto id : players) {
        buffer << id;
    }
    return buffer;
}

/* Reading events operators. */

// Reading event operator.
Buffer &operator>>(Buffer &buffer, Event &event) {
    uint8_t event_type;
    buffer >> event_type;
    if (event_type > 3) {
        throw std::invalid_argument("Wrong event type received");
    }
    event.event_type = (EventType)event_type;
    switch (event.event_type) {
        case BombPlaced:
            buffer >> event.bomb_id >> event.position;
            break;
        case BombExploded:
            buffer >> event.bomb_id >> event.robots_destroyed >> event.blocks_destroyed;
            break;
        case PlayerMoved:
            buffer >> event.player_id >> event.position;
            break;
        case BlockPlaced:
            buffer >> event.position;
            break;
    }
    return buffer;
}

// Writing event operator.
Buffer &operator<<(Buffer &buffer, const Event &event) {
    buffer << (uint8_t)event.event_type;
    switch (event.event_type) {
        case BombPlaced:
            buffer << event.bomb_id << event.position;
            break;
        case BombExploded:
            buffer << event.bomb_id << event.robots_destroyed << event.blocks_destroyed;
            break;
        case PlayerMoved:
            buffer << event.player_id << event.position;
            break;
        case BlockPlaced:
            buffer << event.position;
            break;
    }
    return buffer;
}

// Read events vector operator.
Buffer &operator>>(Buffer &buffer, std::vector<Event> &events) {
    size_t size = buffer.readUint32();
    events.clear();
    for (size_t i = 0; i < size; i++) {
        Event event;
        buffer >> event;
        events.push_back(event);
    }
    return buffer;
}

// Write events vector operator.
Buffer &operator<<(Buffer &buffer, const std::vector<Event> &events) {
    buffer << (uint32_t)events.size();
    for (const auto &event : events) {
        buffer << event;
    }
    return buffer;
}

/* Reading and writing actual messages that client and server will receive or send. */

// Writing message to gui operator.
Buffer &operator<<(Buffer &buffer, const MessageToGui &message) {
    buffer << (uint8_t)message.msg_type;
    switch (message.msg_type) {
        case Lobby:
            buffer << message.server_name << message.player_count << message.size_x
                   << message.size_y << message.game_length << message.explosion_radius
                   << message.bomb_timer << message.players;
            break;
        case Game:
            buffer << message.server_name << message.size_x << message.size_y << message.game_length
                   << message.turn << message.players << message.player_positions << message.blocks
                   << message.bombs << message.explosions << message.scores;
            break;
    }
    return buffer;
}

// Read message from gui operator.
Buffer &operator>>(Buffer &buffer, GuiInputMessage &message) {
    uint8_t msg_type = buffer.readUint8();
    if (msg_type > 2) {
        throw std::invalid_argument("Wrong message type received");
    }
    message.msg_type = (GuiInputEnum)msg_type;
    if (message.msg_type == GuiInputEnum::MoveGui) {
        buffer >> message.direction;
    }
    buffer.assertEnd();
    return buffer;
}

// Write client message operator.
Buffer &operator<<(Buffer &buffer, const ClientMessage &message) {
    buffer << (uint8_t)(message.msg_type);
    if (message.msg_type == Join) {
        buffer << message.player_name;
    } else if (message.msg_type == Move) {
        buffer << message.direction;
    }
    return buffer;
}

// Read client message operator.
Buffer &operator>>(Buffer &buffer, ClientMessage &message) {
    uint8_t msg_type = buffer.readUint8();
    if (msg_type > 3) {
        throw std::invalid_argument("Wrong message type received");
    }
    message.msg_type = (ClientMessageEnum)msg_type;
    if (message.msg_type == Join) {
        buffer >> message.player_name;
    } else if (message.msg_type == Move) {
        buffer >> message.direction;
    }
    return buffer;
}

// Reading server message operator.
Buffer &operator>>(Buffer &buffer, ServerMessage &message) {
    uint8_t msg_type = buffer.readUint8();
    if (msg_type > 4) {
        throw std::invalid_argument("Wrong message type received");
    }
    message.msg_type = (ServerMessageEnum)msg_type;
    switch (message.msg_type) {
        case Hello:
            buffer >> message.server_name >> message.player_count >> message.size_x >>
                message.size_y >> message.game_length >> message.explosion_radius >>
                message.bomb_timer;
            break;
        case AcceptedPlayer:
            buffer >> message.player_id >> message.player;
            break;
        case GameStarted:
            buffer >> message.players;
            break;
        case Turn:
            buffer >> message.turn >> message.events;
            break;
        case GameEnded:
            buffer >> message.scores;
            break;
    }
    return buffer;
}

// Writing server message operator.
Buffer &operator<<(Buffer &buffer, const ServerMessage &message) {
    buffer << (uint8_t)message.msg_type;
    switch (message.msg_type) {
        case Hello:
            buffer << message.server_name << message.player_count << message.size_x
                   << message.size_y << message.game_length << message.explosion_radius
                   << message.bomb_timer;
            break;
        case AcceptedPlayer:
            buffer << message.player_id << message.player;
            break;
        case GameStarted:
            buffer << message.players;
            break;
        case Turn:
            buffer << message.turn << message.events;
            break;
        case GameEnded:
            buffer << message.scores;
            break;
    }
    return buffer;
}

#endif  // BOMBERMAN_SERIALIZATION_HPP
