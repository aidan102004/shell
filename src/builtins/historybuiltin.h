#pragma once

#include <vector>
#include <string>
#include <map>
#include <functional>

class HistoryBuiltin {
    std::vector<std::string> history;
    int index;
    std::string path = "shell_history.txt";
    //flag consts
    using FlagHandler = std::function<void(const std::string&)>; //alias for function pointer
    static inline std::map<std::string, FlagHandler> flag_handlers;//create map of function pointers
    void register_handlers();

public:
    HistoryBuiltin();
    void startup(const std::string& path);
    void handle_builtin(const std::vector<std::string>& tokens);
    void print_history(int n);
    void handle_read(const std::string& file_name);
    void handle_write(const std::string& file_name);
    void handle_append(const std::string& file_name);
    void handle_clear(const std::string& arg);
    void handle_delete(const std::string& arg);
    void add(const std::string& command);
    void close();
    std::string previous();
    std::string next();
};