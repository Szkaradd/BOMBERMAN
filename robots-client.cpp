#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <utility>
#include <map>
#include "utils.hpp"

#define MAX_UDP_BUFF_SIZE 65507
#define DEFAULT_TCP_MSG_SIZE 1024

namespace po = boost::program_options;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

struct launch_settings {
    std::string gui_address;
    std::string server_address;
    std::string player_name;
    uint16_t port{};

    launch_settings() = default;

    launch_settings(std::string  ga, std::string sa, std::string  pn, uint16_t p)
            : gui_address(std::move(ga)), server_address(std::move(sa)),
            player_name(std::move(pn)), port(p) {};
};

enum GameState {
    InLobby, InGame, Spectator, NotConnected
};

// Global variable for storing current game state for the client.
GameState current_state = NotConnected;

enum ClientMessage {
    Join, PlaceBomb, PlaceBlock, Move
};

enum ServerMessage {
    Hello, AcceptedPlayer, GameStarted, Turn, GameEnded
};

enum Direction {
    up, right, down, left
};

enum InputMessage {
    PlaceBombGui, PlaceBlockGui, MoveGui
};

class Lobby {
public:
    Lobby() = default;

    Lobby(std::string sn, uint8_t pc,
          uint16_t sx, uint16_t sy, uint16_t gl,
          uint16_t er, uint16_t bt) :
    server_name(std::move(sn)), players_count(pc),
    size_x(sx), size_y(sy), game_length(gl),
    explosion_radius(er), bomb_timer(bt) {
        players = {};
    };

    void add_player(player_id_t PlayerId, const Player& player) {
        players.insert({PlayerId, player});
    }

    size_t lobby_size() {
        size_t res = 3 * sizeof(uint8_t) + server_name.length() * sizeof(char) +
                5 * sizeof(uint16_t) + sizeof(uint32_t);
        res += calculate_players_map_size(players);
        return res;
    }

    void serialize_message(size_t &length, char message[]) {
        length = lobby_size();

        char *pointer = message;

        uint8_t msg_id = 0;
        modify_message_and_increase_pointer(pointer, &msg_id, sizeof(uint8_t));

        auto s_name_len = (uint8_t) server_name.length();
        modify_message_and_increase_pointer(pointer, &s_name_len, sizeof(uint8_t));
        modify_message_and_increase_pointer(pointer, (void*)server_name.c_str(),
                                            server_name.length() * sizeof(char));
        modify_message_and_increase_pointer(pointer, &players_count, sizeof(uint8_t));

        uint16_t size_x_to_send = htons(size_x);
        modify_message_and_increase_pointer(pointer, &size_x_to_send, sizeof(uint16_t));

        uint16_t size_y_to_send = htons(size_y);
        modify_message_and_increase_pointer(pointer, &size_y_to_send, sizeof(uint16_t));

        uint16_t game_length_to_send = htons(game_length);
        modify_message_and_increase_pointer(pointer, &game_length_to_send, sizeof(uint16_t));

        uint16_t radius_to_send = htons(explosion_radius);
        modify_message_and_increase_pointer(pointer, &radius_to_send, sizeof(uint16_t));

        uint16_t timer_to_send = htons(bomb_timer);
        modify_message_and_increase_pointer(pointer, &timer_to_send, sizeof(uint16_t));

        serialize_players_map(pointer, players);
    }

private:
    std::string server_name;
    uint8_t players_count{};
    uint16_t size_x{};
    uint16_t size_y{};
    uint16_t game_length{};
    uint16_t explosion_radius{};
    uint16_t bomb_timer{};
    std::map<player_id_t , Player> players;
};

class Game {
public:
    Game() = default;

    Game(std::string sn, uint16_t sx, uint16_t sy, uint16_t gl, uint16_t t) :
    server_name(std::move(sn)), size_x(sx), size_y(sy), game_length(gl), turn(t){
        players = {};
        player_positions = {};
        blocks = {};
        bombs = {};
        explosions = {};
        scores = {};
    };

    void add_player_at_pos(player_id_t PlayerId, const Player& player, Position position) {
        players.insert({PlayerId, player});
        player_positions.insert({PlayerId, position});
    }

    void add_player(player_id_t PlayerId, const Player& player) {
        players.insert({PlayerId, player});
    }

    void set_score(player_id_t PlayerId, score_t score) {
        scores.insert({PlayerId, score});
    }

    size_t game_size() {
        size_t res = 4 * sizeof(uint16_t) + 2 * sizeof(uint8_t) + server_name.length() * sizeof(char);
        res += calculate_players_map_size(players);
        res += calculate_players_position_map_size(player_positions);
        res += calculate_positions_list_size(blocks);
        res += calculate_bombs_list_size(bombs);
        res += calculate_positions_list_size(explosions);
        res += calculate_scores_map_size(scores);
        return res;
    }

    void serialize_message(size_t &length, char message[]) {
        length = game_size();

        char *pointer = message;

        uint8_t msg_id = 1;
        modify_message_and_increase_pointer(pointer, &msg_id, sizeof(uint8_t));

        auto s_name_len = (uint8_t) server_name.length();
        modify_message_and_increase_pointer(pointer, &s_name_len, sizeof(uint8_t));
        modify_message_and_increase_pointer(pointer, (void*)server_name.c_str(),
                                            server_name.length() * sizeof(char));

        uint16_t size_x_to_send = htons(size_x);
        modify_message_and_increase_pointer(pointer, &size_x_to_send, sizeof(uint16_t));

        uint16_t size_y_to_send = htons(size_y);
        modify_message_and_increase_pointer(pointer, &size_y_to_send, sizeof(uint16_t));

        uint16_t game_length_to_send = htons(game_length);
        modify_message_and_increase_pointer(pointer, &game_length_to_send, sizeof(uint16_t));

        uint16_t turn_to_send = htons(turn);
        modify_message_and_increase_pointer(pointer, &turn_to_send, sizeof(uint16_t));

        serialize_players_map(pointer, players);
        serialize_players_positions(pointer, player_positions);
        serialize_positions_list(pointer, blocks);
        serialize_bombs_list(pointer, bombs);
        serialize_positions_list(pointer, explosions);
        serialize_scores_map(pointer, scores);
    }

private:
    std::string server_name;
    uint16_t size_x{};
    uint16_t size_y{};
    uint16_t game_length{};
    uint16_t turn{};
    std::map<player_id_t , Player> players;
    std::map<player_id_t , Position> player_positions;
    std::vector<Position> blocks;
    std::vector<Bomb> bombs;
    std::vector<Position> explosions;
    std::map<player_id_t , score_t> scores;
};

launch_settings check_parameters_and_fill_settings(int argc, char *argv[]) {
    std::string gui_address;
    std::string server_address;
    std::string player_name;
    uint16_t port = 0;

    try {
        po::options_description description("Allowed options");

        description.add_options()
                ("help,h", "produce help message")
                ("gui-address,d", po::value<std::string>(&gui_address)->required(), "specify gui address")
                ("Player-name,n", po::value<std::string>(&player_name)->required(), "set Player name")
                ("server-address,s", po::value<std::string>(&server_address)->required(), "specify server address")
                ("port,p", po::value<uint16_t>(&port)->required(), "set client port to listen from gui");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, description), vm);

        if (vm.count("help")) {
            std::cout << "Usage: ./robots-client [options]\n";
            std::cout << description;
            exit(EXIT_SUCCESS);
        }

        po::notify(vm);
    }
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
        exit(EXIT_FAILURE);
    }

    auto settings = launch_settings(gui_address, server_address, player_name, port);
    return settings;
}

void printf_settings(const launch_settings& settings) {
    std::cout << "port: " << settings.port << "\ngui-address: " <<
              settings.gui_address << "\nserver-address: " << settings.server_address <<
              "\nPlayer-name: " << settings.player_name << "\n";
}

Game example_game() {
    Game game = Game("przykladowy serwer", 10, 10, 1000, 0);
    game.add_player_at_pos(1, {"agata", "127.0.0.1:3333"}, {1, 1});
    game.add_player_at_pos(2, {"mikolaj", "127.0.0.1:2222"}, {2, 2});
    game.add_player_at_pos(3, {"zosia", "127.0.0.1:1111"}, {3, 3});
    game.set_score(1, 0);
    game.set_score(2, 0);
    game.set_score(3, 0);
    return game;
}

void receive_from_gui_send_to_server(tcp::socket &tcp_socket, udp::socket &udp_socket) {
    try {
        while (true) {
            char msg_from_gui[MAX_UDP_BUFF_SIZE];
            char msg_to_server[DEFAULT_TCP_MSG_SIZE];
            size_t server_msg_size = 1;
            bool correct_msg_from_gui = true;

            udp::endpoint sender_endpoint;
            size_t reply_length = udp_socket.receive_from(
                    boost::asio::buffer(msg_from_gui, MAX_UDP_BUFF_SIZE), sender_endpoint);

            if (reply_length <= 2) {
                auto input_message = (uint8_t) msg_from_gui[0];
                std::cout << "Received: ";
                switch (input_message) {
                    case PlaceBombGui:
                        std::cout << "PlaceBomb";
                        msg_to_server[0] = PlaceBomb;
                        break;
                    case PlaceBlockGui:
                        std::cout << "PlaceBlock";
                        msg_to_server[0] = PlaceBlock;
                        break;
                    case MoveGui: {
                        auto direction = (uint8_t) msg_from_gui[1];
                        msg_to_server[0] = Move;
                        msg_to_server[1] = (char)direction;
                        server_msg_size = 2;
                        std::cout << "Move " << (int) direction;
                        break;
                    }
                    default:
                        std::cerr << "Wrong message";
                        correct_msg_from_gui = false;
                        break;
                }
                std::cout << " from gui.\n";
            } else {
                correct_msg_from_gui = false;
                std::cerr << "Wrong message from gui\n";
            }

            if (correct_msg_from_gui) {
                boost::system::error_code error;
                boost::asio::write(tcp_socket, boost::asio::buffer(msg_to_server, server_msg_size), error);
            }
        }
    }
    catch (std::exception &e) {
        std::cerr << "error " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

Lobby create_lobby_from_hello_message(char *message, size_t len) {
    if (len == 1) {
        std::cerr << "Wrong message size.\n";
        exit(EXIT_FAILURE);
    }
    char *pointer = message;
    pointer += sizeof(uint8_t); // Skip message type byte.

    auto server_name_size = (uint8_t) (*pointer);
    pointer += sizeof(uint8_t);
    size_t expected_len = 5 * sizeof(uint16_t) + 3 * sizeof(uint8_t) + server_name_size;
    if (len != expected_len) {
        std::cerr << "Wrong message size.\n";
        exit(EXIT_FAILURE);
    }

    char server_name[server_name_size];
    strncpy(server_name, pointer, server_name_size);
    pointer += server_name_size;
    std::string server_name_string(server_name);

    auto players_count = (uint8_t) (*pointer);
    pointer += sizeof(uint8_t);
    uint16_t size_x = ntohs(*((uint16_t *) pointer));
    pointer += sizeof(uint16_t);
    uint16_t size_y = ntohs(*((uint16_t *) pointer));
    pointer += sizeof(uint16_t);
    uint16_t game_length = ntohs(*((uint16_t *) pointer));
    pointer += sizeof(uint16_t);
    uint16_t explosion_radius = ntohs(*((uint16_t *) pointer));
    pointer += sizeof(uint16_t);
    uint16_t bomb_timer = ntohs(*((uint16_t *) pointer));
    pointer += sizeof(uint16_t);

    return {server_name_string, players_count,
                 size_x, size_y, game_length,
                 explosion_radius, bomb_timer};
}

void receive_from_server_send_to_gui(tcp::socket &tcp_socket, udp::socket &udp_socket) {
    try {
        char message_from_server[DEFAULT_TCP_MSG_SIZE];
        size_t len = tcp_socket.receive(boost::asio::buffer(message_from_server));
        if (len == 0) {
            std::cerr << "Error in receiving message from server.\n";
        }
        printf ("size is %zu\n", len);
        printf("Message is: %s\n", message_from_server);
        for (size_t i = 0; i < len; i++) {
            std::cout << message_from_server[i] + 0 << " ";
        }
        std::cout << '\n';

        Lobby lobby;
        Game game;
        std::cerr << "Received ";
        switch (message_from_server[0]) {
            case Hello:
                lobby = create_lobby_from_hello_message(message_from_server, len);
                std::cout << "Hello from server.\n";
                current_state = InLobby;
                break;
            case AcceptedPlayer:
                break;
            case GameStarted:
                std::cout << "GameStarted from server.\n";
                if (current_state != InLobby) current_state = Spectator;
                break;
            case Turn:
                break;
            case GameEnded:
                current_state = InLobby;
                break;
            default:
                std::cerr << "wrong message from server.\n";
                exit(EXIT_FAILURE);
        }
    }
    catch (std::exception &e) {
        std::cerr << "error " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    launch_settings settings = check_parameters_and_fill_settings(argc, argv);

    address_info server_address = get_address_info(settings.server_address);
    address_info gui_address = get_address_info(settings.gui_address);

    boost::asio::io_context io_context;
    tcp::resolver tcp_resolver(io_context);

    auto endpoints =
            tcp_resolver.resolve(server_address.address, server_address.port);

    tcp::socket tcp_socket(io_context/*, tcp::endpoint(tcp::v6(), settings.port)*/);
    tcp_socket.set_option(tcp::no_delay(true));

    boost::asio::connect(tcp_socket, endpoints); // Connect to server.

    udp::socket udp_socket(io_context, udp::endpoint(udp::v6(), settings.port));

    udp::resolver udp_resolver(io_context);
    auto udp_endpoints =
            udp_resolver.resolve(gui_address.address, gui_address.port);

    receive_from_gui_send_to_server(tcp_socket, udp_socket);

    // handle_connection_with_gui(io_context, gui_address, settings.port);

    /*Lobby lobby = Lobby("przykladowy serwer", 3, 10, 10, 200, 4, 5);
    lobby.add_player(0, {"agata", "127.0.0.1:4567"});
    lobby.add_player(1, {"mikolaj", "127.0.0.1:6989"});
    lobby.add_player(2, {"żółtek!", "127.0.0.1:7000"});

    lobby.serialize_message(request_length, request);*/

    /*printf ("size is %zu\n", request_length);
    printf("Message is: %s\n", request);
    for (size_t i = 0; i < request_length; i++) {
        std::cout << request[i] + 0 << " ";
    }
    std::cout << '\n';*/

    /*boost::asio::connect(tcp_socket, endpoints);

    while(true) {
        // Listen for messages
        std::array<char, 128> buf {};
        boost::system::error_code error;

        size_t len = tcp_socket.read_some(boost::asio::buffer(buf), error);

        if (error == boost::asio::error::eof) {
            // Clean connection cut off
            break;
        } else if (error){
            throw boost::system::system_error(error);
        }

        std::cout.write(buf.data(), len);
    }*/

    return 0;
}