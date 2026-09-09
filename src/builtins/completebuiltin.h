#pragma once

#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>
#include "../headers/util.h"

namespace fs = std::__fs::filesystem;

class CompleteBuiltin {
private:
    std::unordered_map<std::string, fs::directory_entry> complete_paths;
public:
    void handle_complete_builtin(std::vector<std::string>& args);
    std::string run_completer(const fs::path& script, const std::string& command, const std::string& curr_word, const std::string& full_input, int tab_count);
    std::vector<std::string> execute_completer(const fs::path& script, const std::string& command, const std::string& curr_word, const std::string& prev_word, const std::string& f_in);
};