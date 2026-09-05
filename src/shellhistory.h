#pragma once

#include <vector>
#include <string>


class ShellHistory {
    std::vector<std::string> history;
    int index;
    
    public:
    void handle_builtin(const std::vector<std::string>& tokens) const {
        int s = 0;
        int pad = 4;
        if (tokens.size() > 1) s = history.size() - std::stoi(tokens[1]);
        for (size_t i = s; i < history.size(); i++) {
            std::cout << std::setw(pad) << i << " " << history[i] << std::endl;
        }
    }

    void add(const std::string& command) {
        if (!command.empty()) {
            history.push_back(command);
            index = history.size();
        }
    }

    std::string previous() {
        if (index == 0) return "";
        index--;
        std::cout << "\r\033[K";
        std::cout << "$ " << history[index] << std::flush;
        return history[index];

    }

    std::string next() {
        if (index == history.size()) return "";
        index++;
        std::cout << "\r\033[K";
        std::cout << "$ " << history[index] << std::flush;
        return history[index];
    }
};