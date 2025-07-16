#include <boost/asio.hpp>
#include <boost/array.hpp> // для boost::array
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <chrono>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

// -------------------------------
// Функции логирования
// -------------------------------

std::string GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&in_time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void Log(const std::string& message) {
    std::string time = GetCurrentTime();
    std::cout << "[" << time << "] " << message << std::endl;
}

void LogError(const std::string& message) {
    std::string time = GetCurrentTime();
    std::cerr << "[" << time << "] ERROR: " << message << std::endl;
}

// -------------------------------
// UDP-ответчик (обнаружение сервера)
// -------------------------------

void BroadcastResponder(boost::asio::io_context& io) {
    try {
        boost::asio::ip::udp::socket socket(io, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 50000));
        socket.set_option(boost::asio::socket_base::broadcast(true));
        boost::array<char, 128> recv_buf;
        boost::asio::ip::udp::endpoint remote_endpoint;

        Log("UDP responder started on port 50000");

        while (true) {
            socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);
            std::string request(recv_buf.data(), recv_buf.size());
            if (request.find("DISCOVER") == 0) {
                std::string local_ip = socket.local_endpoint().address().to_string();
                Log("Received DISCOVER from " + remote_endpoint.address().to_string());
                socket.send_to(boost::asio::buffer("IP:" + local_ip), remote_endpoint);
                Log("Sent IP: " + local_ip);
            }
        }
    } catch (const std::exception& ex) {
        LogError(std::string("UDP responder error: ") + ex.what());
    }
}

// -------------------------------
// RSA-реализация
// -------------------------------

unsigned long long mod_exp(unsigned long long base, unsigned long long exp, unsigned long long mod) {
    unsigned long long result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) result = (result * base) % mod;
        exp /= 2;
        base = (base * base) % mod;
    }
    return result;
}

std::vector<unsigned long long> parse_numbers(const std::string& str) {
    std::vector<unsigned long long> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        result.push_back(std::stoull(item));
    }
    return result;
}

std::string vector_to_string(const std::vector<unsigned long long>& vec) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1) oss << ",";
    }
    return oss.str();
}

unsigned long long mod_inverse(unsigned long long a, unsigned long long m) {
    a = a % m;
    for (unsigned long long x = 1; x < m; ++x) {
        if ((a * x) % m == 1) return x;
    }
    throw std::runtime_error("Inverse doesn't exist");
}

std::string decrypt_string(const std::vector<unsigned long long>& encrypted, unsigned long long d, unsigned long long n) {
    std::string decrypted;
    for (unsigned long long c : encrypted) {
        decrypted += static_cast<char>(mod_exp(c, d, n));
    }
    return decrypted;
}

// -------------------------------
// Клиент и сервер (HandleClient)
// -------------------------------

struct Client {
    std::string username;
    tcp::socket socket;
    unsigned long long e;
    unsigned long long n;

    Client(boost::asio::io_context& io) : socket(io), e(0), n(0) {}
};

std::vector<Client*> clients;
std::mutex clientsMutex;

bool SendPacket(tcp::socket& socket, const std::string& data) {
    try {
        uint32_t length = htonl(static_cast<uint32_t>(data.size()));
        boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
        boost::asio::write(socket, boost::asio::buffer(data));
        return true;
    } catch (const std::exception& ex) {
        LogError(std::string("SendPacket error: ") + ex.what());
        return false;
    }
}

bool ReceivePacket(tcp::socket& socket, std::string& outData) {
    try {
        uint32_t length;
        boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));
        length = ntohl(length);

        outData.resize(length);
        boost::asio::read(socket, boost::asio::buffer(outData.data(), length));
        return true;
    } catch (const std::exception& ex) {
        LogError(std::string("ReceivePacket error: ") + ex.what());
        return false;
    }
}

void HandleClient(Client* client) {
    try {
        std::string message;
        Log("New client connected. Waiting for username...");

        if (!ReceivePacket(client->socket, message)) {
            LogError("Failed to receive username.");
            delete client;
            return;
        }

        if (message.find("USERNAME:") == 0) {
            client->username = message.substr(9);
            tcp::endpoint endpoint = client->socket.remote_endpoint();
            std::string ip = endpoint.address().to_string();
            unsigned short port = endpoint.port();

            Log("Client registered: " + client->username + " (" + ip + ":" + std::to_string(port) + ")");
        } else {
            LogError("Invalid username message from client.");
            delete client;
            return;
        }

        if (!ReceivePacket(client->socket, message)) {
            LogError("Failed to receive public key.");
            delete client;
            return;
        }

        if (message.find("PUBKEY:") == 0) {
            auto parts = parse_numbers(message.substr(7));
            client->e = parts[0];
            client->n = parts[1];
            Log(client->username + " public key registered: e=" + std::to_string(client->e) + ", n=" + std::to_string(client->n));
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(client);
        }

        while (true) {
            std::string packet;
            if (!ReceivePacket(client->socket, packet)) {
                Log(client->username + " disconnected.");
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    auto it = std::find(clients.begin(), clients.end(), client);
                    if (it != clients.end()) clients.erase(it);
                }
                delete client;
                return;
            }

            if (packet.find("GETKEY:") == 0) {
                std::string target = packet.substr(7);
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto c : clients) {
                    if (c->username == target && c->socket.is_open()) {
                        std::string key = "PUBKEY:" + std::to_string(c->e) + "," + std::to_string(c->n);
                        SendPacket(client->socket, key);
                        break;
                    }
                }
            } else if (packet.find("MSG:") == 0) {
                size_t colonPos = packet.find(':', 4);
                if (colonPos == std::string::npos) continue;

                std::string target = packet.substr(4, colonPos - 4);
                std::string encryptedData = packet.substr(colonPos + 1);

                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto c : clients) {
                    if (c->username == target && c->socket.is_open()) {
                        SendPacket(c->socket, encryptedData);
                        break;
                    }
                }
            } else if (packet == "LIST:") {
                std::string userList = "======= USERLIST ========\n";
                std::lock_guard<std::mutex> lock(clientsMutex);
                int index = 1;
                for (auto c : clients) {
                    tcp::endpoint endpoint = c->socket.remote_endpoint();
                    std::string ip = endpoint.address().to_string();
                    unsigned short port = endpoint.port();
                    userList += std::to_string(index++) + " : \t" + c->username + " (" + ip + ":" + std::to_string(port) + ")\n";
                }
                SendPacket(client->socket, "LIST:" + userList);
            } else if (packet.find("MSGALL:") == 0) {
                size_t firstColon = packet.find(':', 7);
                if (firstColon == std::string::npos) continue;

                std::string senderUsername = packet.substr(7, firstColon - 7);
                std::string encryptedData = packet.substr(firstColon + 1);

                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto c : clients) {
                    if (c->socket.is_open() && c->username != senderUsername) {
                        SendPacket(c->socket, "MSGALL:" + senderUsername + ":" + encryptedData);
                    }
                }
            }
        }
    } catch (const std::exception& ex) {
        LogError(std::string("Connection error: ") + ex.what());
    }
}

int main() {
    try {
        boost::asio::io_context io_context;

        // Запуск UDP-ответчика
        std::thread udp_thread([&io_context]() {
            BroadcastResponder(io_context);
        });
        udp_thread.detach();

        // Запуск TCP-сервера
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        Log("TCP server started on port 12345");

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            Log("TCP connection accepted. Creating client...");

            Client* client = new Client(io_context);
            client->socket = std::move(socket);
            std::thread(HandleClient, client).detach();
        }
    } catch (const std::exception& ex) {
        LogError(std::string("Server error: ") + ex.what());
    }

    return 0;
}