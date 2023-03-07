#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <utility>
#include <map>
#include "definitions.hpp"
#include "utils.hpp"
#include "buffer.hpp"
#include "serialization.hpp"

#ifndef NDEBUG
const bool debug = true;
#else
const bool debug = false;
#endif

namespace po = boost::program_options;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

// Struct for storing data from the command line.
struct client_parameters {
    std::string gui_address;
    std::string server_address;
    std::string player_name;
    uint16_t port{};

    client_parameters() = default;

    client_parameters(std::string  ga, std::string sa, std::string  pn, uint16_t p)
            : gui_address(std::move(ga)), server_address(std::move(sa)),
            player_name(std::move(pn)), port(p) {};
};

// Enum describing current status of the game.
enum GameState {
    InLobby, InGame, SendJoinMsg
};

// Atomic global variable describing current game state.
std::atomic<GameState> game_state(SendJoinMsg);

// Class for storing all information about client.
// It stores all network-wise data.
class ClientInfo {
public:
    boost::asio::io_context io_context;

    address_info server_address;
    address_info gui_address;
    client_parameters settings;
    udp::socket gui_socket{io_context};
    udp::endpoint gui_endpoint{};
    udp::resolver UDP_resolver{io_context};
    tcp::socket server_socket{io_context};
    tcp::endpoint server_endpoint{};
    tcp::resolver TCP_resolver{io_context};

    // Constructor attempts to connect with
    // server specified in command line options.
    explicit ClientInfo(client_parameters l_settings) {
        settings = std::move(l_settings);
        server_address = get_address_info(settings.server_address);
        gui_address = get_address_info(settings.gui_address);

        gui_endpoint = *UDP_resolver.resolve(gui_address.address, gui_address.port);
        gui_socket = udp::socket(io_context,
                                 udp::endpoint(udp::v6(), settings.port));

        server_endpoint = *TCP_resolver.resolve(server_address.address, server_address.port);

        if (debug) std::cerr << "Attempting to connect with " << server_endpoint << '\n';
        server_socket.connect(server_endpoint);
        server_socket.set_option(tcp::no_delay(true));
        std::cout << "Connected with " << server_endpoint << '\n';
        std::cout << "Listening gui at " << gui_endpoint << '\n';
    }
};

// Create game settings from command line params.
// If params are incorrect specify error message and exit.
// If parameter -h [--help] was passed - produce help message.
client_parameters check_parameters_and_fill_settings(int argc, char *argv[]) {
    std::string gui_address;
    std::string server_address;
    std::string player_name;
    uint16_t port = 0;

    try {
        po::options_description description("Allowed options");

        description.add_options()
                ("help,h", "produce help message")
                ("gui-address,d", po::value<std::string>(&gui_address)->required(),
                        "specify gui address")
                ("player-name,n", po::value<std::string>(&player_name)->required(),
                        "set Player name")
                ("server-address,s", po::value<std::string>(&server_address)->required(),
                        "specify server address")
                ("port,p", po::value<uint16_t>(&port)->required(),
                        "set client port to listen from gui");

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

    auto settings = client_parameters(gui_address, server_address, player_name, port);
    return settings;
}

// Function produces client-server message from GUI input.
static ClientMessage MessageToServerFromGuiMessage(const GuiInputMessage &m) {
    ClientMessage msg;
    msg.msg_type = (ClientMessageEnum)(m.msg_type + 1);
    if (msg.msg_type == Move) {
        msg.direction = m.direction;
    }
    return msg;
}

// Function produces join message from string with player name.
static ClientMessage JoinMessage(std::string pn) {
    ClientMessage msg;
    msg.msg_type = Join;
    msg.player_name = std::move(pn);
    return msg;
}

void print_message_from_gui(GuiInputMessage m) {
    std::cerr << "Received ";
    switch (m.msg_type) {
        case PlaceBombGui:
            std::cerr << "PlaceBomb";
            break;
        case PlaceBlockGui:
            std::cerr << "PlaceBlock";
            break;
        case MoveGui:
            std::cerr << "Move ";
            printDirection(m.direction);
            break;
    }
    std::cerr << " from gui\n";
}

// Function waits for input from gui, after receiving
// correct message it sends appropriate message to server.
// Function works in infinite loop.
void receive_from_gui_send_to_server(ClientInfo &client_info) {
    try {
        TCPBuffer tcpBuffer(client_info.server_socket);
        UDPBuffer udpBuffer(client_info.gui_socket, client_info.gui_endpoint);
        GuiInputMessage msg_from_gui;
        ClientMessage msg_to_server;

        while (true) {
            try {
                udpBuffer.receiveMsg(0);
                udpBuffer >> msg_from_gui;

                if (debug) print_message_from_gui(msg_from_gui);

                if (game_state == SendJoinMsg) {
                    game_state = InLobby;
                    msg_to_server = JoinMessage(client_info.settings.player_name);
                    tcpBuffer << msg_to_server;
                } else {
                    msg_to_server = MessageToServerFromGuiMessage(msg_from_gui);
                    tcpBuffer << msg_to_server;
                }
                tcpBuffer.sendMsg();
            }
            catch (std::exception &e) {
                std::cerr << "error " << e.what() << '\n';
                continue;
            }
        }
    }
    catch (std::exception &e) {
        std::cerr << "error " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

// Function sets msg_to_gui with appropriate data from hello msg.
void handle_hello_msg(ServerMessage &server_message, MessageToGui &msg_to_gui) {
    if (debug) std::cerr << "Received Hello";
    game_state = SendJoinMsg;
    msg_to_gui.msg_type = Lobby;
    msg_to_gui.server_name = server_message.server_name;
    msg_to_gui.player_count = server_message.player_count;
    msg_to_gui.size_x = server_message.size_x;
    msg_to_gui.size_y = server_message.size_y;
    msg_to_gui.game_length = server_message.game_length;
    msg_to_gui.explosion_radius = server_message.explosion_radius;
    msg_to_gui.bomb_timer = server_message.bomb_timer;
}

// Function sets msg_to_gui with appropriate data from accepted player msg.
void handle_accepted_player(ServerMessage &server_message, MessageToGui &msg_to_gui) {
    msg_to_gui.players.insert({server_message.player_id, server_message.player});
    msg_to_gui.scores[server_message.player_id] = 0;
    if (debug) std::cerr << "Accepted player " << server_message.player.player_name
              << " with address: " << server_message.player.player_address;
}

// Function sets msg_to_gui with appropriate data from game started msg.
void handle_game_started(ServerMessage &server_message, MessageToGui &msg_to_gui) {
    if (debug) std::cerr << "Received GameStarted";
    game_state = InGame;
    msg_to_gui.players = server_message.players;
    for (const auto& player : msg_to_gui.players) {
        msg_to_gui.scores[player.first] = 0;
    }
}

// Function sets msg_to_gui with appropriate data from bomb exploded event.
// It also adds some elements to dead players and destroyed blocks.
void handle_bomb_exploded(MessageToGui &msg_to_gui,
                          std::set<player_id_t> &dead_players,
                          std::set<Position> &destroyed_blocks, const Event &event) {
    Bomb bomb = msg_to_gui.bombs[event.bomb_id];
    Position pos = bomb.position;
    msg_to_gui.bombs.erase(event.bomb_id);
    for (uint16_t i = 0 ; i <= msg_to_gui.explosion_radius; i++) {
        if (pos.x + i == msg_to_gui.size_x) break;
        msg_to_gui.explosions.insert({(uint16_t) (pos.x + i), pos.y});
        if (msg_to_gui.blocks.contains({(uint16_t) (pos.x + i), pos.y})) break;
    }
    for (uint16_t i = 0 ; i <= msg_to_gui.explosion_radius; i++) {
        msg_to_gui.explosions.insert({(uint16_t) (pos.x - i), pos.y});
        if (pos.x - i == 0) break;
        if (msg_to_gui.blocks.contains({(uint16_t) (pos.x - i), pos.y})) break;
    }
    for (uint16_t i = 0 ; i <= msg_to_gui.explosion_radius; i++) {
        if (pos.y + i == msg_to_gui.size_y) break;
        msg_to_gui.explosions.insert({pos.x, (uint16_t) (pos.y + i)});
        if (msg_to_gui.blocks.contains({pos.x, (uint16_t) (pos.y + i)})) break;
    }
    for (uint16_t i = 0 ; i <= msg_to_gui.explosion_radius; i++) {
        msg_to_gui.explosions.insert({pos.x, (uint16_t) (pos.y - i)});
        if (pos.y - i == 0) break;
        if (msg_to_gui.blocks.contains({pos.x, (uint16_t) (pos.y - i)})) break;
    }

    for (auto id: event.robots_destroyed) {
        if (!dead_players.contains(id)) {
            msg_to_gui.scores[id]++;
            dead_players.insert(id);
        }
    }
    for (auto position: event.blocks_destroyed) {
        destroyed_blocks.insert(position);
    }
}

// Function sets msg_to_gui with appropriate data from turn msg.
void handle_turn(ServerMessage &server_message, MessageToGui &msg_to_gui,
                 std::set<player_id_t> &dead_players,
                 std::set<Position> &destroyed_blocks) {
    if (debug) std::cerr << "Received Turn " << server_message.turn;
    for (auto &elem : msg_to_gui.bombs) {
        elem.second.timer--;
    }
    msg_to_gui.explosions.clear();
    msg_to_gui.msg_type = Game;
    msg_to_gui.turn = server_message.turn;
    dead_players.clear();
    destroyed_blocks.clear();
    for (const auto& event : server_message.events) {
        switch (event.event_type) {
            case BombPlaced: {
                Bomb bomb(event.position, msg_to_gui.bomb_timer);
                msg_to_gui.bombs.insert({event.bomb_id, bomb});
                break;
            }
            case BombExploded: {
                handle_bomb_exploded(msg_to_gui,dead_players,
                                     destroyed_blocks, event);
                break;
            }
            case PlayerMoved:
                msg_to_gui.player_positions[event.player_id] = event.position;
                break;
            case BlockPlaced:
                msg_to_gui.blocks.insert(event.position);
                break;
        }
    }
    for (auto position: destroyed_blocks) {
        msg_to_gui.blocks.erase(position);
    }

}

// Function sets msg_to_gui with appropriate data from accepted player msg.
void handle_game_ended(ServerMessage &server_message, MessageToGui &msg_to_gui) {
    if (debug) std::cerr << "Received Game Ended";
    msg_to_gui.msg_type = Lobby;
    game_state = SendJoinMsg;
    msg_to_gui.players.clear();
    msg_to_gui.blocks.clear();
    msg_to_gui.bombs.clear();
    msg_to_gui.scores = server_message.scores;
}

// Function sets msg_to_gui fields depending on server message.
// It analyzes server message.
void message_to_gui_from_server_msg(ServerMessage &server_message, MessageToGui &msg_to_gui) {
    std::set<player_id_t> dead_players;
    std::set<Position> destroyed_blocks;

    switch (server_message.msg_type) {
        case Hello:
            handle_hello_msg(server_message, msg_to_gui);
            break;
        case AcceptedPlayer:
            handle_accepted_player(server_message, msg_to_gui);
            break;
        case GameStarted:
            handle_game_started(server_message, msg_to_gui);
            break;
        case Turn:
            handle_turn(server_message, msg_to_gui, dead_players, destroyed_blocks);
            break;
        case GameEnded:
            handle_game_ended(server_message, msg_to_gui);
            break;
        default:
            std::cerr << "Received wrong message type from server\n";
            exit(EXIT_FAILURE);
    }
    if (debug) std::cerr << " from server\n";
}

// Function works in infinite loop.
// It receives message from server and parses it.
// After that if message is correct it sends appropriate message to gui.
void receive_from_server_send_to_gui(ClientInfo &client_info) {
    try {
        TCPBuffer tcpBuffer(client_info.server_socket);
        UDPBuffer udpBuffer(client_info.gui_socket, client_info.gui_endpoint);
        MessageToGui msg_to_gui;
        ServerMessage msg_from_server;
        while (true) {
            tcpBuffer >> msg_from_server;

            message_to_gui_from_server_msg(msg_from_server, msg_to_gui);
            if (msg_from_server.msg_type != GameStarted) {
                udpBuffer << msg_to_gui;
                udpBuffer.sendMsg();
            }
        }
    }
    catch (std::exception &e){
        std::cerr << "error " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    catch (...) {
        std::cerr << "Unknown exception\n";
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    try {
        client_parameters settings = check_parameters_and_fill_settings(argc, argv);
        ClientInfo client_info(settings);

        std::thread gui_listener_thread(receive_from_gui_send_to_server, std::ref(client_info));
        std::thread server_listener_thread(receive_from_server_send_to_gui, std::ref(client_info));

        gui_listener_thread.join();
        server_listener_thread.join();
    }
    catch (std::exception &e) {
        std::cerr << "error " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    return 0;
}