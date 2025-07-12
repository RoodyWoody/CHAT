#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

using boost::asio::ip::tcp;

struct Client
{
    std::string username;
    tcp::socket socket;

    Client(boost::asio::io_context& io) : socket(io) {};
};

std::vector<Client*> clients;
std::mutex clientsMutex;

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

void HandleClient(Client* client) {
    try {
        std::string message;
        if (!ReceivePacket(client->socket, message)) {
            std::cerr << "Failed register client" << std::endl;
            delete client;
            return;
        }

        if (message.find("USERNAME:") == 0) {
            client->username = message.substr(9); // Извлечение имени
            std::cout << "New client connected: " << client->username << std::endl;
        } else {
            std::cerr << "Invalid username message." << std::endl;
            delete client;
            return;
        }

        // Добавляем клиента в список
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(client);
        }

        while (true) {
            if (!ReceivePacket(client->socket, message)) {
                std::cout << client->username << " disconnected." << std::endl;
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    auto it = std::find(clients.begin(), clients.end(), client);
                    if (it != clients.end()) {
                        clients.erase(it);
                    }
                }
                delete client;
                return;
            }

            if(message == "/list")
            {
                std::string userList = "======= USERLIST ========\n";
                u_int8_t count = 0; 
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto c : clients)
                {
                    count++;
                    tcp::endpoint endpoint = c->socket.remote_endpoint();
                    std::string ip = endpoint.address().to_string();
                    unsigned short port = endpoint.port();
                    userList += std::to_string(count) + " : \t" + c->username + " (" + ip + ":" + std::to_string(port) + ")\n";
                    
                }
                SendPacket(client->socket, userList);
            }
            else
            {
                std::cout << client->username << " sent: " << message << std::endl;

                std::string fullMessage = client->username + ": " + message;
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    for (auto с : clients) {
                        if (с != client) {
                            SendPacket(с->socket, fullMessage); // Отправка через SendPacket
                        }
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

            Client* client = new Client(io_context);
            client->socket = std::move(socket);
            std::thread(HandleClient, client).detach();
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
    }

    return 0;
}