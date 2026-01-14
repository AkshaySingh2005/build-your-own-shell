#include "completion.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <vector>
#include <string>
#include <cstring>

static std::vector<std::string> builtins = {
    "echo" , "cd" , "pwd" , "type","exit"
};

char* command_generator(const char* text, int state) {
    static size_t index;

    if (state == 0)
        index = 0;

    while (index < builtins.size()) {
        const std::string& cmd = builtins[index++];
        if (cmd.rfind(text, 0) == 0) {
            return strdup(cmd.c_str());  
        }
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