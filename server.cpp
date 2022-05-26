#include <iostream>
#include <boost/asio.hpp>
#include "buffer.hpp"
#include "serialization.hpp"

using boost::asio::ip::tcp;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage ./server <port>\n";
        exit(EXIT_FAILURE);
    }
    try {
        boost::asio::io_context io_context;

        auto port = (uint16_t)std::stoi(argv[1]);
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v6(), port));
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        std::cout << "Accepting connections on port " << port << '\n';
        std::string s = socket.remote_endpoint().address().to_string();
        std::cout << "Client " << s << " connected! Sending message!\n";

        TCPBuffer buffer(socket);
        ClientMessage message;

        while(true) {
            //char buf[1024];
            //boost::system::error_code error;

            //size_t len = socket.receiveMsg(boost::asio::buffer(buf));
            buffer >> message;
            //printf ("size is %zu\n", );
            /*printf("Message is: %s\n", buf);
            for (size_t i = 0; i < len; i++) {
                std::cout << buf[i] + 0 << " ";
            }
            std::cout << '\n';*/
            std::cout << "Received ";
            switch (message.msg_type) {
                case Join:
                    std::cout << "Join " << message.player_name;
                    break;
                case PlaceBomb:
                    std::cout << "Place Bomb";
                    break;
                case PlaceBlock:
                    std::cout << "Place Block";
                    break;
                case Move:
                    std::cout << "Move ";
                    printDirection(message.direction);
                    break;
            }
            std::cout << '\n';

           // std::string hello_message = "Hello, beautiful client!\n";

            //boost::asio::write(socket, boost::asio::buffer(hello_message), error);
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}