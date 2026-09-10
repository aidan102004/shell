#pragma once

#include <iostream>
#include <unordered_map>
#include <string>
#include <fstream>
#include "trie.h"

namespace fs = std::__fs::filesystem;

class Completer {
private:
    static std::unordered_map<std::string, fs::directory_entry> complete_paths;
public:
    static std::string completion(Trie& trie, std::string cur_input, const std::string& full_line, int tab_count);
    static std::string path_completion(const std::string& s, const std::string& full_line, size_t s_pos, int tab_count);
    static bool find_path(const std::string& query);
    static std::filesystem::path get_path(const std::string& key);
};