#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <chrono>
#include <iomanip>

using boost::asio::ip::tcp;

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

unsigned long long mod_inverse(unsigned long long a, unsigned long long m) {
    a = a % m;
    for (unsigned long long x = 1; x < m; ++x) {
        if ((a * x) % m == 1)
            return x;
    }
    throw std::runtime_error("Inverse doesn't exist");
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

std::vector<unsigned long long> encrypt_string(const std::string& message, unsigned long long e, unsigned long long n) {
    std::vector<unsigned long long> encrypted;
    for (char c : message) {
        encrypted.push_back(mod_exp(static_cast<unsigned long long>(c), e, n));
    }
    return encrypted;
}

std::string decrypt_string(const std::vector<unsigned long long>& encrypted, unsigned long long d, unsigned long long n) {
    std::string decrypted;
    for (unsigned long long c : encrypted) {
        decrypted += static_cast<char>(mod_exp(c, d, n));
    }
    return decrypted;
}

// -------------------------------
// Вспомогательные функции
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

bool SendPacket(tcp::socket& socket, const std::string& data) {
    try {
        uint32_t length = htonl(static_cast<uint32_t>(data.size()));
        boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
        boost::asio::write(socket, boost::asio::buffer(data));
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "SendPacket error: " << ex.what() << std::endl;
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
        std::cerr << "ReceivePacket error: " << ex.what() << std::endl;
        return false;
    }
}

bool ConnectToServer(boost::asio::io_context& io, tcp::socket& socket) {
    try {
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", "12345");
        boost::asio::connect(socket, endpoints);
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "Connection failed: " << ex.what() << std::endl;
        return false;
    }
}

void showHelp() {
    std::cout << "Available commands:\n";
    std::cout << "  /whisp <name> <message>   - Send encrypted message to specific user\n";
    std::cout << "  /all <message>             - Send encrypted message to all users\n";
    std::cout << "  /list                      - Show connected users\n";
    std::cout << "  /help                      - Show this help\n";
    std::cout << "  <message>                  - Same as /all\n";
}

void ReceiveMessages(tcp::socket& socket, unsigned long long d, unsigned long long n) {
    std::string packet;
    while (true) {
        if (!ReceivePacket(socket, packet)) {
            std::cout << "Disconnected from server." << std::endl;
            break;
        }

        if (packet.find("MSGALL:") == 0) {
            size_t firstColon = packet.find(':', 7);
            if (firstColon == std::string::npos) continue;

            std::string sender = packet.substr(7, firstColon - 7);
            std::string encryptedStr = packet.substr(firstColon + 1);

            std::vector<unsigned long long> encrypted = parse_numbers(encryptedStr);
            std::string decrypted = decrypt_string(encrypted, d, n);
            Log(sender + " (all): " + decrypted);
        } else if (packet.find("LIST:") == 0) {
            std::cout << packet.substr(5) << std::endl;
        } else {
            std::vector<unsigned long long> encrypted = parse_numbers(packet);
            std::string decrypted = decrypt_string(encrypted, d, n);
            Log("Received: " + decrypted);
        }
    }
}

int main() {
    try {
        boost::asio::io_context io;
        tcp::socket socket(io);

        std::string username;
        std::cout << "Enter your name: ";
        std::getline(std::cin, username);

        // Генерация RSA-ключей
        unsigned long long p = 61;
        unsigned long long q = 53;
        unsigned long long n = p * q;
        unsigned long long phi = (p - 1) * (q - 1);
        unsigned long long e = 17;
        unsigned long long d = mod_inverse(e, phi); // 2753

        while (true) {
            if (ConnectToServer(io, socket)) {
                SendPacket(socket, "USERNAME:" + username);

                std::string publicKey = "PUBKEY:" + std::to_string(e) + "," + std::to_string(n);
                SendPacket(socket, publicKey);

                Log("Connected and public key sent.");

                std::thread(ReceiveMessages, std::ref(socket), d, n).detach();

                std::string inputLine;
                while (true) {
                    std::cout << "> ";
                    std::getline(std::cin, inputLine);

                    if (inputLine.length() != 0)
                    {
                        if (inputLine.find("/whisp ") == 0) {
                        size_t firstSpace = inputLine.find(' ', 7);
                        if (firstSpace == std::string::npos) continue;

                        std::string target = inputLine.substr(7, firstSpace - 7);
                        std::string message = inputLine.substr(firstSpace + 1);

                        SendPacket(socket, "GETKEY:" + target);

                        std::string response;
                        if (!ReceivePacket(socket, response) || response.find("PUBKEY:") != 0) {
                            std::cerr << "Failed to get public key" << std::endl;
                            continue;
                        }

                        auto parts = parse_numbers(response.substr(7));
                        unsigned long long receiver_e = parts[0];
                        unsigned long long receiver_n = parts[1];

                        std::vector<unsigned long long> encrypted = encrypt_string(message, receiver_e, receiver_n);
                        std::string encryptedStr = vector_to_string(encrypted);
                        SendPacket(socket, "MSG:" + target + ":" + encryptedStr);

                        } else if (inputLine.find("/all ") == 0) {
                            std::string message = inputLine.substr(5);

                            std::vector<unsigned long long> encrypted = encrypt_string(message, e, n);
                            std::string encryptedStr = vector_to_string(encrypted);
                            SendPacket(socket, "MSGALL:" + username + ":" + encryptedStr);

                        } else if (inputLine == "/list") {
                            SendPacket(socket, "LIST:");
                            continue;

                        } else if (inputLine == "/help") {
                            showHelp();
                            continue;

                        } else if (inputLine == "/exit") {
                            socket.close();
                            break;

                        } else {
                            // По умолчанию отправляем всем
                            std::vector<unsigned long long> encrypted = encrypt_string(inputLine, e, n);
                            std::string encryptedStr = vector_to_string(encrypted);
                            SendPacket(socket, "MSGALL:" + username + ":" + encryptedStr);
                        }
                    }  
                }
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}