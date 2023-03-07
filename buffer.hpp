#ifndef BOMBERMAN_BUFFER_HPP
#define BOMBERMAN_BUFFER_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <utility>

// Constants for buffer sizes.
static const size_t UDP_BUFF_SIZE = 65507;
static const size_t TCP_BUFF_SIZE = 1024;

// Class buffer for reading and writing network messages.
class Buffer {
protected:
    char *buff;
    size_t size;
    size_t read_cursor = 0, write_cursor = 0;

    explicit Buffer(size_t s) : size(s) {
        buff = new char[s];
    }

    // Function make sure that there is enough space in the buffer to write n bytes.
    virtual void ensureThatWriteIsPossible([[maybe_unused]]size_t to_write) {};
    // Function make sure that there will be enough bytes to read.
    virtual void ensureThatReadIsPossible([[maybe_unused]]size_t to_read) {};

public:
    virtual void receiveMsg([[maybe_unused]]size_t to_receive) {};
    virtual void sendMsg() {};

    // Functions for writing and reading standard data types into the buffer.
    // Each time we write we add sizeof(read variable) to write cursor.
    // Each time we read we add sizeof(read variable) to read cursor.

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

// Buffer for sending and reading UDP messages.
class UDPBuffer : public Buffer {
    boost::asio::ip::udp::socket &udp_socket;
    boost::asio::ip::udp::endpoint udp_endpoint;

public:
    UDPBuffer(boost::asio::ip::udp::socket & socket, boost::asio::ip::udp::endpoint  endpoint) :
            Buffer(UDP_BUFF_SIZE), udp_socket(socket), udp_endpoint(std::move(endpoint)) {}

    // If we receive message we treat write cursor as message size.
    void receiveMsg([[maybe_unused]]size_t to_receive) override {
        read_cursor = 0;
        write_cursor = udp_socket.receive(boost::asio::buffer(buff, size));
    }

    void sendMsg() override {
        udp_socket.send_to(boost::asio::buffer
                (buff, write_cursor), udp_endpoint);
        read_cursor = 0;
        write_cursor = 0;
    }
};

// Buffer for sending and reading TCP messages.
class TCPBuffer : public Buffer {
    boost::asio::ip::tcp::socket &tcp_socket;

    // Function checks if it is possible to read to_read bytes from buffer.
    // If it exceeded buff size we move buffer contents to the left.
    void ensureThatReadIsPossible(const size_t to_read) override {
        if (read_cursor + to_read > size) {
            for (size_t i = 0; i < write_cursor - read_cursor; i++) {
                buff[i] = buff[read_cursor + i];
            }
            write_cursor -= read_cursor;
            read_cursor = 0;
        }
        // If buffer content size is smaller than to_read we need another message.
        if (write_cursor - read_cursor < to_read) {
            receiveMsg(to_read - (write_cursor - read_cursor));
        }
    }

    // If we exceeded buffer size we just send the message.
    void ensureThatWriteIsPossible(const size_t to_write) override {
        if (write_cursor + to_write > size) {
            sendMsg();
        }
    }

public:
    explicit TCPBuffer(boost::asio::ip::tcp::socket &socket) :
            Buffer(TCP_BUFF_SIZE), tcp_socket(socket) {
    }

    // Receive exactly to_receive bytes.
    void receiveMsg(size_t to_receive) override {
        boost::system::error_code error;
        boost::asio::read(
                tcp_socket, boost::asio::buffer(buff + write_cursor, to_receive), error
        );
        if (error == boost::asio::error::eof) {
            throw std::invalid_argument("Connection closed by peer");
        } else if (error) {
            throw boost::system::system_error(error);
        }
        write_cursor += to_receive;
    }

    // Send message with all buffer contents.
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
