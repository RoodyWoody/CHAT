#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <mutex>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

std::mutex socketMutex;
SOCKET clientSocket = INVALID_SOCKET;

int ReceiveAll(SOCKET sock, char* buffer, size_t totalBytes) {
    size_t received = 0;
    while (received < totalBytes) {
        int bytes = recv(sock, buffer + received, totalBytes - received, 0);
        if (bytes <= 0) return bytes;
        received += bytes;
    }
    return received;
}

bool SendPacket(SOCKET sock, const std::string& data) {
    uint32_t length = static_cast<uint32_t>(data.size());
    if (send(sock, reinterpret_cast<const char*>(&length), sizeof(length), 0) <= 0)
        return false;
    if (send(sock, data.c_str(), data.size(), 0) <= 0)
        return false;
    return true;
}

bool ReceivePacket(SOCKET sock, std::string& outData) {
    uint32_t length = 0;
    if (ReceiveAll(sock, reinterpret_cast<char*>(&length), sizeof(length)) <= 0)
        return false;
    outData.resize(length);
    if (ReceiveAll(sock, &outData[0], length) <= 0)
        return false;
    return true;
}

void ReceiveMessages() {
    std::string packet;
    while (true) {
        if (!ReceivePacket(clientSocket, packet)) {
            std::cout << "Disconnected from server. Reconnecting..." << std::endl;
            break;
        }
        std::cout << packet << std::endl;
    }
}

bool ConnectToServer() {
    std::lock_guard<std::mutex> lock(socketMutex);
    if (clientSocket != INVALID_SOCKET) closesocket(clientSocket);

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) return false;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    if (inet_pton(AF_INET, "192.168.2.112", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address!" << std::endl;
        return false;
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        std::cerr << "Connect failed with error: " << error << std::endl;
        return false;
    }

    return true;
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    std::string username;
    std::cout << "Enter your name: ";
    std::getline(std::cin, username);
    std::string usernameMsg = "USERNAME:" + username;

    while (true) {
        if (ConnectToServer()) {
            SendPacket(clientSocket, usernameMsg);
            std::thread(ReceiveMessages).detach();

            std::string message;
            while (true) {
                std::getline(std::cin, message);
                if (!SendPacket(clientSocket, message)) {
                    std::cerr << "Failed to send message." << std::endl;
                    break;
                }
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}