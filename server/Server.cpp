#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <algorithm>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// Платформозависимые заголовки и типы
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cstring>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
#endif

std::vector<SOCKET> clients;
std::mutex clientsMutex;

// Логирование с уровнем и цветами
std::mutex logMutex;

enum class LogLevel { INFO, WARNING, ERROR, MESSAGE };

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;

    #ifdef _WIN32
        localtime_s(&tm, &now_time_t); // Windows
    #else
        localtime_r(&now_time_t, &tm); // Linux
    #endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void logMessage(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex); // Потокобезопасность
    std::string timestamp = getCurrentTime();
    std::string levelStr;
    const char* color = "", *reset = "";

    switch (level) {
        case LogLevel::INFO:
            levelStr = "[INFO]";
            #ifndef _WIN32
                color = "\033[32m"; // Green
            #endif
            break;
        case LogLevel::WARNING:
            levelStr = "[WARNING]";
            #ifndef _WIN32
                color = "\033[33m"; // Yellow
            #endif
            break;
        case LogLevel::ERROR:
            levelStr = "[ERROR]";
            #ifndef _WIN32
                color = "\033[31m"; // Red
            #endif
            break;
        case LogLevel::MESSAGE:
            levelStr = "[MESSAGE]";
            #ifndef _WIN32
                color = "\033[34m"; // Blue
            #endif
            break;
    }

    #ifndef _WIN32
        reset = "\033[0m";
    #endif

    std::cout << color << timestamp << " " << levelStr << " " << reset << message << std::endl;
}

// Инициализация Winsock (Windows) или ничего (Linux)
void initNetworking() {
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            logMessage(LogLevel::ERROR, "WSAStartup failed.");
            exit(1);
        }
    #endif
}

// Очистка Winsock (Windows) или ничего (Linux)
void cleanupNetworking() {
    #ifdef _WIN32
        WSACleanup();
    #endif
}

// Функция для безопасного чтения данных
int ReceiveAll(SOCKET sock, char* buffer, size_t totalBytes) {
    size_t received = 0;
    while (received < totalBytes) {
        size_t bytes = recv(sock, buffer + received, totalBytes - received, 0);
        if (bytes <= 0) return bytes;
        received += bytes;
    }
    return received;
}

// Функция для получения пакета
bool ReceivePacket(SOCKET sock, std::string& outData) {
    uint32_t length = 0;
    if (ReceiveAll(sock, reinterpret_cast<char*>(&length), sizeof(length)) <= 0)
        return false;

    outData.resize(length);
    if (ReceiveAll(sock, &outData[0], length) <= 0)
        return false;

    return true;
}

// Функция для отправки пакета
bool SendPacket(SOCKET sock, const std::string& data) {
    uint32_t length = static_cast<uint32_t>(data.size());
    if (send(sock, reinterpret_cast<const char*>(&length), sizeof(length), 0) <= 0)
        return false;

    if (send(sock, data.c_str(), data.size(), 0) <= 0)
        return false;

    return true;
}

void HandleClient(SOCKET clientSocket) {
    std::string username = "Anonymous";

    // Получаем имя клиента
    std::string message;
    if (!ReceivePacket(clientSocket, message)) {
        logMessage(LogLevel::WARNING, "Client disconnected before sending username.");
        closesocket(clientSocket);
        return;
    }

    if (message.find("USERNAME:") == 0) {
        username = message.substr(9); // Извлечение имени
        logMessage(LogLevel::INFO, "New client connected: " + username);
    } else {
        logMessage(LogLevel::ERROR, "Invalid username message.");
    }

    // Добавляем клиента в список
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.push_back(clientSocket);
    }

    while (true) {
        if (!ReceivePacket(clientSocket, message)) {
            logMessage(LogLevel::WARNING, username + " disconnected.");

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                auto it = std::find(clients.begin(), clients.end(), clientSocket);
                if (it != clients.end()) {
                    clients.erase(it);
                }
            }
            closesocket(clientSocket);
            return;
        }

        logMessage(LogLevel::MESSAGE, username + " sent: " + message);

        std::string fullMessage = username + ": " + message;
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (SOCKET client : clients) {
                if (client != clientSocket) {
                    SendPacket(client, fullMessage);
                }
            }
        }
    }
}

int main() {
    initNetworking();

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        logMessage(LogLevel::ERROR, "Socket creation failed.");
        cleanupNetworking();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // Порт
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Все интерфейсы

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        logMessage(LogLevel::ERROR, "Bind failed.");
        closesocket(serverSocket);
        cleanupNetworking();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        logMessage(LogLevel::ERROR, "Listen failed.");
        closesocket(serverSocket);
        cleanupNetworking();
        return 1;
    }

    logMessage(LogLevel::INFO, "Server started. Waiting for connections...");

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            logMessage(LogLevel::ERROR, "Accept failed.");
            continue;
        }

        std::thread(HandleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    cleanupNetworking();
    return 0;
}
