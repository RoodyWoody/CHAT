#include <iostream>
#include <cstdio>
#include <memory>
#include <string>
#include <array>
#include <json.hpp> 

using json = nlohmann::json;

std::string execute_python_script(const std::string& action, const std::string& params_json) {
    std::string command = "python3 db_query.py '" + params_json + "'";
    
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    
    if (!pipe) {
        throw std::runtime_error("Script runtime error");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

int main() {
    std::string host = "postgres";
    std::string port = "5432";
    std::string dbname = "postgres_db";
    std::string user = "admin";
    std::string password = "admin";

    try {
        json insert_data = {
            {"host", host},
            {"port", port},
            {"dbname", dbname},
            {"user", user},
            {"password", password},
            {"action", "insert"},
            {"socket", "192.168.1.1"},
            {"username", "Alice"},
            {"user_password", "secure_pass"}
        };

        std::cout << "Insert user test:\n";
        std::string insert_result = execute_python_script("insert", insert_data.dump());
        std::cout << "Response:\n" << insert_result << "\n\n";

        json select_data = {
            {"host", host},
            {"port", port},
            {"dbname", dbname},
            {"user", user},
            {"password", password},
            {"action", "select"}
        };

        std::cout << "Select user list test:\n";
        std::string select_result = execute_python_script("select", select_data.dump());
        std::cout << "Response:\n" << select_result << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}