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

using namespace std;

/* -------------------- Parsing -------------------- */

vector<string> parse_args(const string& input) {
    vector<string> args;
    string cur;
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
    vector<string> args;
    string file;
    bool redirect = false;
};

Redir split_redirection(const vector<string>& tokens) {
    Redir r;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ">" || tokens[i] == "1>") {
            if (i + 1 < tokens.size()) {
                r.redirect = true;
                r.file = tokens[i + 1];
                break;
            }
        } else {
            r.args.push_back(tokens[i]);
        }
    }
    return r;
}

/* -------------------- Builtins -------------------- */

void echo_cmd(const vector<string>& args) {
    for (size_t i = 1; i < args.size(); i++) {
        cout << args[i];
        if (i + 1 < args.size()) cout << " ";
    }
    cout << endl;
}

void pwd_cmd() {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf))) cout << buf << endl;
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


void type_cmd(const vector<string>& args) {
    if (args.size() < 2) return;
    string cmd = args[1];

    if (cmd == "echo" || cmd == "pwd" || cmd == "cd" || cmd == "type" || cmd == "exit") {
        cout << cmd << " is a shell builtin\n";
        return;
    }

    char* path = getenv("PATH");
    string p(path);
    stringstream ss(p);
    string dir;
    while (getline(ss, dir, ':')) {
        string full = dir + "/" + cmd;
        if (access(full.c_str(), X_OK) == 0) {
            cout << cmd << " is " << full << endl;
            return;
        }
    }
    cout << cmd << ": not found\n";
}

/* -------------------- External -------------------- */

void run_program(const vector<string>& args) {
    pid_t pid = fork();
    if (pid == 0) {
        vector<char*> argv;
        for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        cerr << args[0] << ": command not found\n";
        _exit(1);
    } else {
        waitpid(pid, nullptr, 0);
    }
}

/* -------------------- Shell -------------------- */

int main() {
    cout << unitbuf;
    cerr << unitbuf;

    while (true) {
        cout << "$ ";
        string input;
        if (!getline(cin, input)) break;
        if (input == "exit") break;

        auto tokens = parse_args(input);
        auto r = split_redirection(tokens);
        if (r.args.empty()) continue;

        int saved = -1;
        if (r.redirect) {
            int fd = open(r.file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) { perror("open"); continue; }
            saved = dup(STDOUT_FILENO);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        string cmd = r.args[0];
        if (cmd == "echo") echo_cmd(r.args);
        else if (cmd == "pwd") pwd_cmd();
        else if (cmd == "cd") cd_cmd(r.args);
        else if (cmd == "type") type_cmd(r.args);
        else run_program(r.args);

        if (saved != -1) {
            dup2(saved, STDOUT_FILENO);
            close(saved);
        }
    }
}
