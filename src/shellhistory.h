#pragma once

#include <vector>
#include <string>

class ShellHistory {
    std::vector<std::string> history;
    int index;
    //flag consts
    static constexpr std::string_view FLAG_READ = "-r";
    static constexpr std::string_view FLAG_WRITE = "-w";
    
public:
    void handle_builtin(const std::vector<std::string>& tokens);
    void handle_read(const std::string& file_name);
    void handle_write(const std::string& file_name);
    void add(const std::string& command);
    std::string previous();
    std::string next();
};