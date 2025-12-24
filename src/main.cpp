#include <iostream>
#include <string>



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
      if(input.substr(5) == "echo" || input.substr(5) == "exit" || input.substr(5) == "type"){
        std::cout<<input.substr(5)<<" "<<"is a shell builtin"<<std::endl;
      }
      else{
        std::cout << input.substr(5) << ": not found" << std::endl;
      }
    }

    else{
        std::cout << input << ": command not found" << std::endl;
    }
  
}

}
