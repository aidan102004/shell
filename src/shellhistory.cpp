#include "shellhistory.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <fstream>

namespace fs = std::__fs::filesystem;

ShellHistory::ShellHistory() : index(0) {
    static bool initialised = false;
    if (!initialised) {
        register_handlers();
        startup(path);
        initialised = true;
    }
}
void ShellHistory::startup(const std::string& path) {
    setenv("HISTFILE", path.c_str(), 1);
    handle_read(path);

}
void ShellHistory::register_handlers() {
    flag_handlers["-r"] = [this](const std::string& arg) {handle_read(arg);};
    flag_handlers["-w"] = [this](const std::string& arg) {handle_write(arg);};
    flag_handlers["-a"] = [this](const std::string& arg) {handle_append(arg);};
} 

void ShellHistory::handle_builtin(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        print_history(0);
        return;
    }
    
    std::string flag = tokens[1];
    
    auto it = flag_handlers.find(flag);
    if (it != flag_handlers.end()) {
        if (tokens.size() > 2) {
            it->second(tokens[2]); //runs the function
        } else {
            std::cerr << "Error: " << flag << " requires an argument\n";
        }
        return;
    }
    
    try {
        print_history(history.size() - std::stoi(flag));
    } catch (const std::invalid_argument&) {
        std::cerr << "Error: '" << flag << "' is not a valid flag or number\n";
    }
}

void ShellHistory::print_history(int n) {
    int pad = 4;
    for (size_t i = n; i < history.size(); i++) std::cout << std::setw(pad) << i << " " << history[i] << std::endl;
    
}

void ShellHistory::handle_read(const std::string& file_name) {
    std::ifstream file(file_name);
    std::string str; 
    while (std::getline(file, str))
    {
        history.push_back(str); //add each line into the history vector
    }
    index = history.size();
}

void ShellHistory::handle_write(const std::string& file_name) {
    std::ofstream file(file_name);
    for (const auto& line : history) {
        file << line + "\n";
    }
    file.close();
}

void ShellHistory::handle_append(const std::string& file_name) {
    std::ofstream file(file_name, std::ios_base::app | std::ios_base::out); //combine flags 
    for (const auto& line : history) {
        file << line + "\n";
    }
    file.close();

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

void ShellHistory::close() {
    handle_append(path);
}