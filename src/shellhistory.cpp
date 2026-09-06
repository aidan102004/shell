#include "shellhistory.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <fstream>

namespace fs = std::__fs::filesystem;


void ShellHistory::handle_builtin(const std::vector<std::string>& tokens) {
    int s = 0, pad = 4;
    if (tokens.size() > 1) {
        std::string flag = tokens[1];
        if (flag == FLAG_READ) { //handle read flag
            if (tokens.size() > 2) {
                handle_read(tokens[2]);  //pass the filename
            } else {
                std::cerr << "Error: -r requires an argument" << std::endl;
            }
            return;
        } else if (flag == FLAG_WRITE) {
            if (tokens.size() > 2) {
                handle_write(tokens[2]);  //pass the filename
            } else {
                std::cerr << "Error: -w requires an argument" << std::endl;
            }
            return;
        } else {
            try {
                s = history.size() - std::stoi(tokens[1]);
            } catch (const std::invalid_argument&) {
                std::cerr << "Error: '" << tokens[1] << "' is not a valid number" << std::endl;
                return;
            }
        }
    }
    for (size_t i = s; i < history.size(); i++) {
        std::cout << std::setw(pad) << i << " " << history[i] << std::endl;
    }
}

void ShellHistory::handle_read(const std::string& file_name) {
    std::ifstream file(file_name);
    std::string str; 
    while (std::getline(file, str))
    {
        history.push_back(str); //add each line into the history vector
    }
}

void ShellHistory::handle_write(const std::string& file_name) {
    // implementation
}

void ShellHistory::add(const std::string& command) {
    if (!command.empty()) {
        history.push_back(command);
        index = history.size();
    }
}
std::string ShellHistory::previous() {
    if (index == 0) return "";
    index--;
    std::cout << "\r\033[K";
    std::cout << "$ " << history[index] << std::flush;
    return history[index];
}
std::string ShellHistory::next() {
    if (index == history.size()) return "";
    index++;
    std::cout << "\r\033[K";
    std::cout << "$ " << history[index] << std::flush;
    return history[index];
}