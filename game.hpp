#ifndef BOMBERMAN_GAME_HPP
#define BOMBERMAN_GAME_HPP

#include <boost/asio.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "buffer.hpp"
#include "definitions.hpp"
#include "serialization.hpp"
#include "utils.hpp"

using boost::asio::ip::tcp;

std::string address_from_socket(tcp::socket &socket) {
    std::string s = socket.remote_endpoint().address().to_string();
    uint16_t client_port = socket.remote_endpoint().port();
    std::string client_port_string = std::to_string(client_port);
    s.append(":");
    s.append(client_port_string);
    return s;
}

class Game {
   private:
    server_parameters game_settings;

    boost::asio::io_context io_context;
    tcp::acceptor acceptor{io_context, tcp::endpoint(tcp::v6(), game_settings.port)};

    uint8_t connected_players;
    player_id_t curr_id;
    std::map<player_id_t, Player> players;
    ServerMessage server_message;
    ClientMessage client_message;
    std::map<player_id_t, score_t> scores;

    [[nodiscard]] ServerMessage create_hello_message() const {
        ServerMessage msg;
        msg.msg_type = Hello;
        msg.server_name = game_settings.server_name;
        msg.player_count = game_settings.players_count;
        msg.size_x = game_settings.size_x;
        msg.size_y = game_settings.size_y;
        msg.game_length = game_settings.game_length;
        msg.explosion_radius = game_settings.explosion_radius;
        msg.bomb_timer = game_settings.bomb_timer;
        return msg;
    }

    ServerMessage create_accepted_player_message(Player player) {
        ServerMessage msg;
        msg.msg_type = AcceptedPlayer;
        msg.player_id = curr_id;
        curr_id++;
        msg.player = std::move(player);
        return msg;
    }

    ServerMessage create_game_started_message() {
        ServerMessage msg;
        msg.msg_type = GameStarted;
        msg.players = players;
        return msg;
    }

    ServerMessage create_game_ended_message() {
        ServerMessage msg;
        msg.msg_type = GameEnded;
        msg.scores = scores;
        return msg;
    }

    ServerMessage create_turn_message(uint16_t turn) {
        ServerMessage msg;
        msg.msg_type = Turn;
        msg.turn = turn;
        msg.events = {};

        for (auto player : players) {
            Event event;
            event.event_type = PlayerMoved;
            event.position = {static_cast<uint16_t>(turn % game_settings.size_x),
                              static_cast<uint16_t>(turn % game_settings.size_y)};
            msg.events.push_back(event);
        }
        for (uint16_t i = 0; i < game_settings.initial_blocks; i++) {
            Event event;
            event.event_type = BlockPlaced;
            event.position = {static_cast<uint16_t>(turn % game_settings.size_x),
                              static_cast<uint16_t>(turn % game_settings.size_y)};
            msg.events.push_back(event);
        }

        return msg;
    }

    void handle_connection() {
        tcp::socket socket(io_context);
        TCPBuffer buffer(socket);
        acceptor.accept(socket);

        buffer << create_hello_message();
        buffer.sendMsg();

        std::string client_address = address_from_socket(socket);
        std::cout << "Client " << client_address << " connected!\n";

        while (true) {
            buffer >> client_message;
            std::cout << "Received ";
            switch (client_message.msg_type) {
                case Join:
                    std::cout << "Join " << client_message.player_name;
                    if (connected_players < game_settings.players_count) {
                        Player new_player = {client_message.player_name, client_address};
                        players.insert({curr_id, new_player});
                        buffer << create_accepted_player_message(new_player);
                        buffer.sendMsg();
                    }
                    break;
                case PlaceBomb:
                    std::cout << "Place Bomb";
                    break;
                case PlaceBlock:
                    std::cout << "Place Block";
                    break;
                case Move:
                    std::cout << "Move ";
                    printDirection(client_message.direction);
                    break;
            }
            std::cout << '\n';
            if (client_message.msg_type == Join) break;
        }

        std::cout << "Sending game started to " << client_address << '\n';
        buffer << create_game_started_message();
        buffer.sendMsg();
        for (int i = 0; i < game_settings.game_length; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(game_settings.turn_duration));
            std::cout << "Sending turn " << i << " to " << client_address << '\n';
            buffer << create_turn_message((uint16_t)i);
            buffer.sendMsg();
        }
        std::cout << "Sending game ended to " << client_address << '\n';
        buffer << create_game_ended_message();
        buffer.sendMsg();
    }

   public:
    explicit Game(server_parameters &settings) : game_settings(settings) {
        connected_players = 0;
        curr_id = 0;
    };

    void run_game() {
        std::cout << "Accepting connections on port " << game_settings.port << '\n';

        try {
            while (true) {
                handle_connection();
            }
        } catch (std::exception &e) {
            std::cerr << "error: " << e.what() << '\n';
            run_game();
        }
    }
};

#endif  // BOMBERMAN_GAME_HPP
