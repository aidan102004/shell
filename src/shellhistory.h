#pragma once

#include <vector>
#include <string>
#include <map>
#include <functional>

class ShellHistory {
    std::vector<std::string> history;
    int index;
    //flag consts
    using FlagHandler = std::function<void(const std::string&)>; //alias for function pointer
    static inline std::map<std::string, FlagHandler> flag_handlers;//create map of function pointers
    void register_handlers();

public:
    ShellHistory();
    void handle_builtin(const std::vector<std::string>& tokens);
    void print_history(int n);
    void handle_read(const std::string& file_name);
    void handle_write(const std::string& file_name);
    void handle_append(const std::string& file_name);
    void add(const std::string& command);
    std::string previous();
    std::string next();
};