#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <string>
#include <mutex>

using boost::asio::ip::tcp;

std::mutex socketMutex;
tcp::socket* clientSocket = nullptr;

// Функция отправки пакета
bool SendPacket(tcp::socket& socket, const std::string& data) {
    try {
        uint32_t length = htonl(static_cast<uint32_t>(data.size()));
        boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
        boost::asio::write(socket, boost::asio::buffer(data));
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "SendPacket error: " << ex.what() << std::endl;
        return false;
    }
}

// Функция получения пакета
bool ReceivePacket(tcp::socket& socket, std::string& outData) {
    try {
        uint32_t length;
        boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));
        length = ntohl(length);
        outData.resize(length);
        boost::asio::read(socket, boost::asio::buffer(outData.data(), length));
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "ReceivePacket error: " << ex.what() << std::endl;
        return false;
    }
}

void ReceiveMessages(tcp::socket& socket) {
    std::string packet;
    while (true) {
        if (!ReceivePacket(socket, packet)) {
            std::cout << "Disconnected from server." << std::endl;
            break;
        }
        std::cout << packet << std::endl;
    }
}

bool ConnectToServer(boost::asio::io_context& io, tcp::socket& socket) {
    try {
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", "12345");
        boost::asio::connect(socket, endpoints);
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Connection failed: " << ex.what() << std::endl;
        return false;
    }
}

int main() {
    try {
        boost::asio::io_context io;

        tcp::socket socket(io);
        std::string username;
        std::cout << "Enter your name: ";
        std::getline(std::cin, username);
        std::string usernameMsg = "USERNAME:" + username;

        while (true) {
            if (ConnectToServer(io, socket)) {
                SendPacket(socket, usernameMsg);

                std::thread(ReceiveMessages, std::ref(socket)).detach();

                std::string message;
                while (true) {
                    std::getline(std::cin, message);
                    SendPacket(socket, message);
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}