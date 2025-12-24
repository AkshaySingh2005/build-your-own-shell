#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>      
#include <sstream>      
#include <unistd.h>//POSIX  
#include <sys/stat.h>   

bool is_builtin(const std::string& cmd) {
    return cmd == "exit" || cmd == "echo" || cmd == "type";
}

bool is_executable(const std::string& path) {
    return access(path.c_str(), X_OK) == 0; // 0 -> executable
                                            // -1 -> non-executable file

}

std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> dirs;
    std::stringstream ss(path);
    std::string dir;

    while (std::getline(ss, dir, ':')) { // used delimitter here':'
        dirs.push_back(dir);
    }

    return dirs;
}

void handle_type(const std::string& command) {
    if (is_builtin(command)) {
        std::cout << command << " is a shell builtin" << std::endl;
        return;
    }


    const char* path_env = std::getenv("PATH"); // PATH=/usr/bin:
    if (!path_env) {
        std::cout << command << ": not found" << std::endl;
        return;
    }

    std::string path_str(path_env);
    std::vector<std::string> dirs = split_path(path_str);

   
    for (const auto& dir : dirs) {
        std::string full_path = dir + "/" + command;

        if (is_executable(full_path)) {
            std::cout << command << " is " << full_path << std::endl;
            return;
        }
    }

    std::cout << command << ": not found" << std::endl;
}

int main() {
    // Flush after every std::cout / std:cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    while (true) {
        std::cout << "$ ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            break; 
        }

        if (input == "exit") {
            break;
        }
        else if (input.rfind("echo ", 0) == 0) {
            std::cout << input.substr(5) << std::endl;
        }
        else if (input.rfind("type ", 0) == 0) {
            std::string cmd = input.substr(5);
            handle_type(cmd);
        }
        else {
            std::cout << input << ": command not found" << std::endl;
        }
    }

    return 0;
}
