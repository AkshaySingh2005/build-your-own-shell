#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>      // getenv
#include <sstream>      // stringstream
#include <unistd.h>     //POSIX // fork, execv, access 
#include <sys/wait.h>   // waitpid
#include <cstring>      // strerror
#include <errno.h>      // errno
#include <limits.h>     // PATHMAX 


bool is_builtin(const std::string& cmd) {
    return cmd == "exit" || cmd == "echo" || cmd == "type" || cmd == "pwd" || cmd=="cd";
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

std::vector<std::string> parse_args(const std::string& input) {
    std::vector<std::string> args;
    std::string current;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];

        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        }
        else if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        }
        else if (c == ' ' && !in_single_quote && !in_double_quote) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        }
        else {
            current += c;
        }
    }

    if (!current.empty()) {
        args.push_back(current);
    }

    return args;
}



// Shell → fork → child execs → parent waits
void run_program(const std::string& input){
    std::vector<std::string>args = parse_args(input);
    
    if(args.empty()){
        return;
    }

    const std::string& program = args[0];

    const char* path_env = std::getenv("PATH");
    if (!path_env) {
        return;
    }
    std::vector<std::string> path_dirs = split_path(path_env);

    for (const auto& dir : path_dirs) {
        std::string full_path = dir + "/" + program;

        if (is_executable(full_path)) {
           
            pid_t pid = fork();
            // fork() creates a NEW process
            // 1. Parent shell
            // 2. Child process (copy of shell)
            
            if(pid < 0){
                std::cout << "fork failed: " << strerror(errno) << std::endl;
                continue;
            }
            if(pid == 0){
                std::vector<char*> argv;
                for (auto& arg : args) {
                    argv.push_back(const_cast<char*>(arg.c_str()));
                }
                argv.push_back(nullptr); 

                execv(full_path.c_str(), argv.data());

                std::cout << program << ": command not found" << std::endl;
                _exit(1);
            }
            else{
                waitpid(pid, nullptr, 0);
            }
            return;
        }
    }
    std::cout << program << ": command not found" << std::endl;
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


void pwd_cmd(){
    char buf[PATH_MAX];

    if(getcwd(buf,sizeof(buf)) != nullptr){
        std::cout<<buf<<std::endl;
    }
    else{
        std::cerr<<"pwd: "<<std::strerror(errno)<<std::endl;
    }
}

void cd_cmd(const std::string& path) {
    std::string target;

    if (path.empty() || path == "~") {
        const char* home = std::getenv("HOME");
        if (!home) {
            std::cout << "cd: HOME not set" << std::endl;
            return;
        }
        target = home;
    }
    else {
        target = path;
    }

    if (chdir(target.c_str()) == -1) {
        std::cout << "cd: " << target << ": No such file or directory" << std::endl;
    }
}


void echo_cmd(const std::string& s){
    std::vector<std::string> args = parse_args(s);
    
    for (size_t i = 0; i < args.size(); ++i) {
        std::cout << args[i];
        if (i < args.size() - 1) {
            std::cout << ' ';
        }
    }
    std::cout << std::endl;
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
            std::string cmd = input.substr(5);
            echo_cmd(cmd);
        }

        else if (input.rfind("type ", 0) == 0) {
            std::string cmd = input.substr(5);
            handle_type(cmd);
        }

        else if(input=="pwd"){
            pwd_cmd();
        }

        else if(input.rfind("cd ", 0) == 0){
            std::string path = input.substr(3);
            cd_cmd(path);
        }

        else {
            run_program(input);
        }
    }

    return 0;
}
