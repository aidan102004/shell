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

namespace fs = std::filesystem;

//builtin commands list
std::unordered_set<std::string> commands = {
    "echo", "exit", "type", "pwd", "cd"
};

void handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins);
void handle_cd(const std::string& arg);
std::string find_path(const std::string& arg);
int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    while (true) {
        std::cout << "$ ";
        std::string command;
        std::getline(std::cin, command);
        std::vector<std::string> tokens; //creates string vector to hold parsed input
        std::string cur = "";
        bool iq = false;
        bool idq = false;
        for (size_t i = 0; i<command.size(); i++) {
            char c = command[i];
            if (c == '\\' && !iq && !idq) {  // backslash outside quotes
                if (i + 1 < command.size()) {
                    cur += command[++i];  // skip the backslash, add next char literally
                }
            }
            else if (c == '\\' && idq) {          // backslash inside double quotes
                if (i + 1 < command.size()) {
                    char next = command[i + 1];
                    if (next == '"' || next == '\\') {
                        cur += command[++i];         // only escape " and 
                    } else {
                        cur += c;                    // otherwise keep the backslash literally
                    }
                }
            }
            else if (c == '\"' && !idq && !iq) {
                idq = true;
            }
            else if (c == '\'' && !iq && !idq) {
                iq = true;
            } else if( c== '\'' && iq) {
                iq = false;
            }
            else if (c == '\"' && idq) {
                idq = false;
            } else if (c==' ' && !iq && !idq) {
                if (!cur.empty()) {
                    tokens.push_back(cur); // space outside quotes = end of token
                    cur = "";
                }
            } else {
                cur += c;
            }
        }
        
        if (!cur.empty()) tokens.push_back(cur);
        if (tokens.empty()) continue;
        std::string cmd = tokens[0]; //sets the cmd to the first element
        if (cmd == "exit") {
            break;
        }
        if (cmd == "type") {
          handle_type(tokens[1], commands);
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
        else if (cmd == "pwd") {
            std::cout << fs::current_path().string() << std::endl; //prints out the current path
        }
        else if (cmd == "cd") {
            handle_cd(tokens.size() > 1 ? tokens[1] : ""); //handles cd 
        }
    }
}
void handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins) {
    if (builtins.find(arg) != builtins.end()) {
            std::cout << arg << " is a shell builtin" << std::endl;
            return;
        }
        std::string found_path = find_path(arg);

        if (!found_path.empty()) {
            std::cout << arg << " is " << found_path << std::endl;
        } else {
            std::cout << arg << ": not found" << std::endl;
        }
}
void handle_cd(const std::string& arg) {
    const char* path = arg.c_str(); //converts the arg into a pointer pointing to the first char in the array of chars
    if (arg.substr(0, 1) == "~") {
        path = std::getenv("HOME"); //gets the path to home
    }

    if (chdir(path) != 0) { //chdir returns -1 if it cannot find the directory, otherwise it changes the path
        std::cout << "cd: " << path << ": No such file or directory" << std::endl;
    } 
}
std::string find_path(const std::string& arg) {
    const char* p = std::getenv("PATH"); //gets the path env variable
    if (!p) {
        std::cout << arg << ": not found" << std::endl;
        return "";
    }

    std::string path = p;
    std::stringstream ss(path);
    std::string dir;
    //reads the path into dr array splitting them up :
    while (std::getline(ss, dir, ':')) {

        if (dir.empty()) continue;

        fs::path full_path = dir + "/" + arg; //appends the arg to the end

        if (access(full_path.c_str(), X_OK) == 0) {
            return full_path.string(); //checks the path to see if the file is an executable
        }
    }

    return "";
}