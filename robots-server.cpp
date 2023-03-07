// The server is not finished, but it at least tries to do something useful..

#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "definitions.hpp"
#include "game.hpp"

namespace po = boost::program_options;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

// Create game settings from command line params.
// If params are incorrect specify error message and exit.
// If parameter -h [--help] was passed - produce help message.
server_parameters check_parameters_and_fill_settings(int argc, char *argv[]) {
    server_parameters launch_settings;

    uint16_t players_count_u16;
    try {
        po::options_description description("Allowed options");

        description.add_options()
                ("help,h", "produce help message")
                ("bomb-timer,b", po::value<uint16_t>(&launch_settings.bomb_timer)->required(),
                 "set bomb timer")
                ("players-count,c", po::value<uint16_t>(&players_count_u16)->required(),
                 "set number of players required for game to start")
                ("turn-duration,d", po::value<uint64_t>(&launch_settings.turn_duration)->required(),
                 "set turn duration")
                ("explosion-radius,e", po::value<uint16_t>(&launch_settings.explosion_radius)->required(),
                 "set explosion radius")
                ("initial-blocks,k", po::value<uint16_t>(&launch_settings.initial_blocks)->required(),
                 "set the amount of blocks placed at game start")
                ("game-length,l", po::value<uint16_t>(&launch_settings.game_length)->required(),
                 "set number of turns the game will last")
                ("server-name,n", po::value<std::string>(&launch_settings.server_name)->required(),
                 "set server name")
                ("port,p", po::value<uint16_t>(&launch_settings.port)->required(),
                 "set server port")
                ("seed,s", po::value<uint32_t>(&launch_settings.seed),
                 "set game seed")
                ("size-x,x", po::value<uint16_t>(&launch_settings.size_x)->required(),
                 "set size-x - horizontal dimension of board")
                ("size-y,y", po::value<uint16_t>(&launch_settings.size_y)->required(),
                 "set size-y - vertical dimension of board");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, description), vm);

        if (vm.count("help")) {
            std::cout << "Usage: ./robots-client [options]\n";
            std::cout << description;
            exit(EXIT_SUCCESS);
        }

        po::notify(vm);
        launch_settings.players_count = (uint8_t)players_count_u16;
    }
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
        exit(EXIT_FAILURE);
    }

    return launch_settings;
}

int main(int argc, char* argv[]) {
    server_parameters launch_settings = check_parameters_and_fill_settings(argc, argv);
    class Game game(launch_settings);
    game.run_game();
    return 0;
}