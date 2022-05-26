#ifndef BOMBERMAN_BUFFER_HPP
#define BOMBERMAN_BUFFER_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <utility>

static const size_t UDP_BUFF_SIZE = 65507;
static const size_t TCP_BUFF_SIZE = 1024;

class Buffer {
protected:
    char *buff;
    size_t size;
    size_t read_cursor = 0, write_cursor = 0;

    explicit Buffer(size_t s) : size(s) {
        buff = new char[s];
    }

    virtual void ensureThatWriteIsPossible([[maybe_unused]]size_t to_write) {};
    virtual void ensureThatReadIsPossible([[maybe_unused]]size_t to_read) {};

public:
    virtual void receiveMsg([[maybe_unused]]size_t to_receive) {};
    virtual void sendMsg() {};

    void writeUint8(const uint8_t &value) {
        ensureThatWriteIsPossible(sizeof(uint8_t));
        uint8_t value_to_send = value;
        memcpy(buff + write_cursor, &value_to_send, sizeof(uint8_t));
        write_cursor += sizeof(uint8_t);
    }

    void writeUint16(const uint16_t &value) {
        ensureThatWriteIsPossible(sizeof(uint16_t));
        uint16_t value_to_send = htons(value);
        memcpy(buff + write_cursor, &value_to_send, sizeof(uint16_t));
        write_cursor += sizeof(uint16_t);
    }

    void writeUint32(const uint32_t &value) {
        ensureThatWriteIsPossible(sizeof(uint32_t));
        uint32_t value_to_send = htonl(value);
        memcpy(buff + write_cursor, &value_to_send, sizeof(uint32_t));
        write_cursor += sizeof(uint32_t);
    }

    void writeString(const std::string &value) {
        size_t length = value.length();
        ensureThatWriteIsPossible(length * sizeof(char));
        memcpy(buff + write_cursor, value.c_str(), length);
        write_cursor += length;
    }

    uint8_t readUint8() {
        ensureThatReadIsPossible(sizeof(uint8_t));
        auto retval = *((uint8_t *) (buff + read_cursor));
        read_cursor += sizeof(uint8_t);
        return retval;
    }

    uint16_t readUint16() {
        ensureThatReadIsPossible(sizeof(uint16_t));
        auto retval = ntohs(*((uint16_t *) (buff + read_cursor)));
        read_cursor += sizeof(uint16_t);
        return retval;
    }

    uint32_t readUint32() {
        ensureThatReadIsPossible(sizeof(uint32_t));
        auto retval = ntohl(*((uint32_t *) (buff + read_cursor)));
        read_cursor += sizeof(uint32_t);
        return retval;
    }

    std::string readString(const size_t &length) {
        ensureThatReadIsPossible(length * sizeof(char));
        std::string retval;
        retval.append(buff + read_cursor, length);
        read_cursor += length;
        return retval;
    }

    virtual ~Buffer() {
        delete[] buff;
    }

    // Function checks if there is something left in the buffer.
    // Used when we want to make sure that nothing is in the buffer.
    void assertEnd() const {
        if (write_cursor - read_cursor != 0) {
            throw std::length_error("UDP message is to long");
        }
    }
};

class UDPBuffer : public Buffer {
    boost::asio::ip::udp::socket &udp_socket;
    boost::asio::ip::udp::endpoint udp_endpoint;

public:
    UDPBuffer(boost::asio::ip::udp::socket & socket, boost::asio::ip::udp::endpoint  endpoint) :
            Buffer(UDP_BUFF_SIZE), udp_socket(socket), udp_endpoint(std::move(endpoint)) {}

    void receiveMsg([[maybe_unused]]size_t to_receive) override {
        read_cursor = 0;
        write_cursor = udp_socket.receive(boost::asio::buffer(buff, size));
    }

    void sendMsg() override {
        size_t bytes = udp_socket.send_to(boost::asio::buffer
                (buff, write_cursor), udp_endpoint);
        read_cursor = 0;
        write_cursor = 0;
    }
};

class TCPBuffer : public Buffer {
    boost::asio::ip::tcp::socket &tcp_socket;

    void ensureThatReadIsPossible(const size_t to_read) override {
        if (read_cursor + to_read > size) {
            for (size_t i = 0; i < write_cursor - read_cursor; i++) {
                buff[i] = buff[read_cursor + i];
            }
            write_cursor -= read_cursor;
            read_cursor = 0;
        }
        if (write_cursor - read_cursor < to_read) {
            receiveMsg(to_read - (write_cursor - read_cursor));
        }
    }

    void ensureThatWriteIsPossible(const size_t to_write) override {
        if (write_cursor + to_write > size) {
            sendMsg();
        }
    }

public:
    explicit TCPBuffer(boost::asio::ip::tcp::socket &socket) :
            Buffer(TCP_BUFF_SIZE), tcp_socket(socket) {
    }

    void receiveMsg(size_t to_receive) override {
        boost::system::error_code error;
        boost::asio::read(
                tcp_socket, boost::asio::buffer(buff + write_cursor, to_receive), error
        );
        if (error == boost::asio::error::eof) {
            std::cerr << "Connection closed by peer.\n";
            exit(EXIT_FAILURE);
        } else if (error) {
            throw boost::system::system_error(error);
        }
        write_cursor += to_receive;
    }

    void sendMsg() override {
        if (write_cursor - read_cursor > 0) {
            boost::asio::write(
                    tcp_socket, boost::asio::buffer(buff + read_cursor, write_cursor - read_cursor)
            );
            write_cursor = 0;
            read_cursor = 0;
        }
    }

};

#endif //BOMBERMAN_BUFFER_HPP
