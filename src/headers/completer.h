#pragma once

#include <iostream>
#include <string>
#include "trie.h"

class Completer {
public:
    static std::string completion(Trie& trie, std::string cur_input, const std::string& full_line, int tab_count);
    static std::string path_completion(const std::string& s, const std::string& full_line, size_t s_pos, int tab_count);
};