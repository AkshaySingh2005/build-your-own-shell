#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include "completion.h"
#include <readline/readline.h>
#include <readline/history.h>




/* -------------------- Parsing -------------------- */

std::vector<std::string> parse_args(const std::string& input) {
    std::vector<std::string> args;
    std::string cur;
    bool in_single = false, in_double = false;

    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];

        if (c == '\\') {
            if (!in_single && !in_double && i + 1 < input.size()) {
                cur += input[++i];
                continue;
            }
            if (in_double && i + 1 < input.size() &&
                (input[i + 1] == '"' || input[i + 1] == '\\')) {
                cur += input[++i];
                continue;
            }
        }

        if (c == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }

        if (c == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }

        if (c == ' ' && !in_single && !in_double) {
            if (!cur.empty()) {
                args.push_back(cur);
                cur.clear();
            }
            continue;
        }

        cur += c;
    }

    if (!cur.empty()) args.push_back(cur);
    return args;
}

/* -------------------- Redirection -------------------- */

struct Redir {
    std::vector<std::string> args;

    std::string out_file;
    bool out_redirect = false;

    std::string err_file;
    bool err_redirect = false;

    bool out_append = false;
    bool err_append = false;
};

Redir split_redirection(const std::vector<std::string>& tokens) {
    Redir r;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ">" || tokens[i] == "1>") {
            if (i + 1 < tokens.size()) {
                r.out_redirect = true;
                r.out_file = tokens[i + 1];
                i++;
            }
        }
        else if (tokens[i] == ">>" || tokens[i] == "1>>") {
            if (i + 1 < tokens.size()) {
                r.out_redirect = true;
                r.out_append = true;
                r.out_file = tokens[++i];
            }
        }        
        else if(tokens[i] == "2>"){
            if(i+1 < tokens.size()){
                r.err_redirect = true;
                r.err_file = tokens[i+1];
                i++;
            }
        }
        else if (tokens[i] == "2>>") {
            if (i + 1 < tokens.size()) {
                r.err_redirect = true;
                r.err_append = true;
                r.err_file = tokens[++i];
            }
        }        
        else {
            r.args.push_back(tokens[i]);
        }
    }
    return r;
}

/* -------------------- Builtins -------------------- */

void echo_cmd(const std::vector<std::string>& args) {
    for (size_t i = 1; i < args.size(); i++) {
        std::cout << args[i];
        if (i + 1 < args.size()) std::cout << " ";
    }
    std::cout << std::endl;
}

void pwd_cmd() {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf))) std::cout << buf << std::endl;
}

void cd_cmd(const std::vector<std::string>& args) {
    std::string target;

    if (args.size() == 1) {
        const char* home = getenv("HOME");
        if (!home) {
            std::cout << "cd: HOME not set\n";
            return;
        }
        target = home;
    }
    else {
        std::string path = args[1];

        if (path[0] == '~') {
            const char* home = getenv("HOME");
            if (!home) {
                std::cout << "cd: HOME not set\n";
                return;
            }

            target = std::string(home) + path.substr(1);
        }
        else {
            target = path;
        }
    }

    if (chdir(target.c_str()) == -1) {
        std::cout << "cd: " << target << ": No such file or directory\n";
    }
}


void type_cmd(const std::vector<std::string>& args) {
    if (args.size() < 2) return;
    std::string cmd = args[1];

    if (cmd == "echo" || cmd == "pwd" || cmd == "cd" || cmd == "type" || cmd == "exit" || cmd == "history") {
        std::cout << cmd << " is a shell builtin\n";
        return;
    }

    char* path = getenv("PATH");
    std::string p(path);
    std::stringstream ss(p);
    std::string dir;

    while (getline(ss, dir, ':')) {
        std::string full = dir + "/" + cmd;
        if (access(full.c_str(), X_OK) == 0) {
            std::cout << cmd << " is " << full << std::endl;
            return;
        }
    }
    std::cout << cmd << ": not found\n";
}

void history_cmd(const std::vector<std::string>& args) {
    HIST_ENTRY** hist = history_list();
    if (!hist) return;

    int count = history_length;

  
    if (args.size() == 1) {
        for (int i = 0; i < count; i++) {
            std::cout << i + 1 << "  " << hist[i]->line << "\n";
        }
        return;
    }

    int n = 0;
    try {
        n = std::stoi(args[1]);
    } catch (...) {
        return;
    }

    if (n <= 0) return;

    int start = count - n;
    if (start < 0) start = 0;

    for (int i = start; i < count; i++) {
        std::cout << i + 1 << "  " << hist[i]->line << "\n";
    }
}




/* -------------------- External -------------------- */

void run_program(const std::vector<std::string>& args) {

    pid_t pid = fork();

    if (pid == 0) {
        std::vector<char*> argv;

        for (auto& a : args){
            argv.push_back(const_cast<char*>(a.c_str()));
        } 
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());

        std::cerr << args[0] << ": command not found\n";
        _exit(1);
    } 

    else {
        waitpid(pid, nullptr, 0);
    }
}



/* -------------------- Shell -------------------- */

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    setup_readline();


    while (true) {
        // std::cout << "$ ";
        // std::string input;
        // if (!getline(std::cin, input)) break;

        char* raw = readline("$ ");
        if (!raw) break;

        std::string input(raw);
        free(raw);

        if (!input.empty())
            add_history(input.c_str());

        if (input == "exit") break;

        auto tokens = parse_args(input);

        auto r = split_redirection(tokens);

        if (r.args.empty()) continue;

        int saved_out = -1;
        int saved_err = -1;


        if (r.out_redirect) {
            int flags = O_WRONLY | O_CREAT | (r.out_append ? O_APPEND : O_TRUNC);
            int fd = open(r.out_file.c_str(), flags, 0644);
            if (fd == -1) { perror("open"); continue; }

            saved_out = dup(STDOUT_FILENO);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        if (r.err_redirect) {
            int flags = O_WRONLY | O_CREAT | (r.err_append ? O_APPEND : O_TRUNC);
            int fd = open(r.err_file.c_str(), flags, 0644);
            if (fd == -1) { perror("open"); continue; }

            saved_err = dup(STDERR_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
      
 
        std::string cmd = r.args[0];

        if (cmd == "echo") echo_cmd(r.args);

        else if (cmd == "pwd") pwd_cmd();

        else if (cmd == "cd") cd_cmd(r.args);

        else if (cmd == "type") type_cmd(r.args);

        else if(cmd == "history") history_cmd(r.args);
        
        else run_program(r.args);

        if (saved_out != -1) {
            dup2(saved_out, STDOUT_FILENO);
            close(saved_out);
        }

        if (saved_err != -1) {
            dup2(saved_err, STDERR_FILENO);
            close(saved_err);
        }

    }
}
