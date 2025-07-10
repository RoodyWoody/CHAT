#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

using boost::asio::ip::tcp;

std::vector<tcp::socket*> clients;
std::mutex clientsMutex;

// Функция отправки пакета (length + message)
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

// Функция получения пакета (length + message)
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

void HandleClient(tcp::socket socket) {
    try {
        std::string username = "Anonymous";

        // Получаем имя клиента
        std::string message;
        if (!ReceivePacket(socket, message)) {
            std::cout << "Client disconnected before sending username." << std::endl;
            return;
        }

        if (message.find("USERNAME:") == 0) {
            username = message.substr(9); // Извлечение имени
            std::cout << "New client connected: " << username << std::endl;
        } else {
            std::cerr << "Invalid username message." << std::endl;
        }

        // Добавляем клиента в список
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(&socket);
        }

        while (true) {
            if (!ReceivePacket(socket, message)) {
                std::cout << username << " disconnected." << std::endl;
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    auto it = std::find(clients.begin(), clients.end(), &socket);
                    if (it != clients.end()) {
                        clients.erase(it);
                    }
                }
                return;
            }

            std::cout << username << " sent: " << message << std::endl;

            std::string fullMessage = username + ": " + message;
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto client : clients) {
                    if (client != &socket) {
                        SendPacket(*client, fullMessage); // Отправка через SendPacket
                    }
                }
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Connection error: " << ex.what() << std::endl;
    }
}

int main() {
    try {
        boost::asio::io_context io_context;

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Server started. Waiting for connections..." << std::endl;

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            std::thread(HandleClient, std::move(socket)).detach();
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
    }

    return 0;
}