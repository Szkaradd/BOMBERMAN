#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

enum { max_length = 1024 };

void server(boost::asio::io_context& io_context, unsigned short port)
{
    udp::socket sock(io_context, udp::endpoint(udp::v4(), port));
    for (;;)
    {
        char data[max_length];
        udp::endpoint sender_endpoint;
        size_t length = sock.receive_from(
                boost::asio::buffer(data, max_length), sender_endpoint);
        std::cout << "Received " << length << " bytes from " << sender_endpoint << "\n";
        printf("%.*s\n", (int)length, data);
        sock.send_to(boost::asio::buffer(data, length), sender_endpoint);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: blocking_udp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        server(io_context, std::atoi(argv[1]));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}