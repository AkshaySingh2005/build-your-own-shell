#include "completion.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <vector>
#include <string>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <set>
#include <cstdlib>

static std::vector<std::string> builtins = {
    "echo" , "cd" , "pwd" , "type","exit"
};

char* command_generator(const char* text, int state) {
    static std::vector<std::string> matches;
    static size_t index;

    if (state == 0) {
        matches.clear();
        index = 0;

        std::set<std::string> seen;

        // Builtins 
        for (const auto& cmd : builtins) {
            if (cmd.rfind(text, 0) == 0) {
                seen.insert(cmd);
            }
        }

        // PATH executables 
        const char* path_env = getenv("PATH");
        if (path_env) {
            std::string path(path_env);
            size_t start = 0;

            while (true) {
                size_t end = path.find(':', start);
                std::string dir = path.substr(start, end - start);

                DIR* dp = opendir(dir.c_str());
                if (dp) {
                    struct dirent* entry;
                    while ((entry = readdir(dp)) != nullptr) {
                        std::string name = entry->d_name;

                        if (name.rfind(text, 0) != 0)
                            continue;

                        std::string full = dir + "/" + name;
                        if (access(full.c_str(), X_OK) == 0) {
                            seen.insert(name);
                        }
                    }
                    closedir(dp);
                }

                if (end == std::string::npos)
                    break;
                start = end + 1;
            }
        }

        matches.assign(seen.begin(), seen.end());
    }

    if (index < matches.size()) {
        return strdup(matches[index++].c_str());
    }

    return nullptr;
}


char** completion(const char* text, int start, int end) {
    (void)end;

    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }

    return nullptr;
}

void setup_readline() {
    rl_attempted_completion_function = completion;
}