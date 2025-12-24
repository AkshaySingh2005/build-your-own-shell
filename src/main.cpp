#include <iostream>
#include <string>
#include <vector>
#include <unistd.h> //POSIX
#include <sstream>
#include <cstdlib>
#include <sys/stat.h> 



bool is_builtin(const std::string& cmd){
   return cmd=="exit" || cmd =="echo" || cmd == "type";
}

bool is_executable(const std::string& path){
  return access(path.c_str(),X_OK) == 0; // 0 -> executable file
                                         // -1 -> non-executable file
}

std::vector<std::string>split_path(const std::string& path){
  std::vector<std::string>dirs;
  std::stringstream ss(path);
  std::string dir;
  
  while(std::getline(ss,dir,':')){
    dirs.push_back(dir);
  }
  gr
  return dirs;
}

void handle_type(const std::string& cmd){

  if(is_builtin(cmd)){
    std::cout<<cmd<<" is a shell builtin"<<std::endl;
    return;
  }

  const char* path_env = std::getenv("PATH"); // PATH=/usr/bin:
  if (!path_env) {
    std::cout << cmd << ": not found" << std::endl;
    return;
  }

  std::string path_str(path_env);
  std::vector<std::string> dirs = split_path(path_str);

  for(const auto& dir:dirs){
    std::string full_path = dir + '/' + cmd;

    if(is_executable(full_path)){
       std::cout << cmd << " is " << full_path << std::endl;
       return;
    }
  }
  std::cout << cmd << ": not found" << std::endl;
}


int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  

  while (true) {
    std::cout << "$ ";

    std::string input;
    std::getline(std::cin, input);

    if (input == "exit") {
        break;
    }

    else if(input.substr(0,4) == "echo"){
      std::cout<<input.substr(5)<<std::endl;
    }

    else if(input.substr(0,4) == "type"){
       std::string cmd = input.substr(5);
       handle_type(cmd);

    }
    else{
        std::cout << input << ": command not found" << std::endl;
    }
  
}

}
