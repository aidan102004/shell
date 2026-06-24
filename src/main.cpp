#include <iostream>
#include <string>
#include <unordered_set>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

//builtin commands list
std::unordered_set<std::string> commands = {
    "echo", "exit", "type", "pwd", "cd"
};

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    while (true) {
        std::cout << "$ ";
        std::string command;
        std::getline(std::cin, command);

        std::vector<std::string> tokens; //creates string vector to hold parsed input
        std::string cmd = tokens[0]; //sets the cmd to the first element
        if (cmd == "exit") {
            break;
        }
        else if (cmd == "echo") {
            
            for(size_t i = 1; i <tokens.size(); i++) {
                if (i > 1) {
                std::cout << " ";
                std::cout << tokens[i];
                }
                std::cout << std::endl;
            }
        }
    }
}