#include "./headers/util.h"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace fs = std::__fs::filesystem;

std::string Util::matches_helper(std::vector<std::string>& matches, const std::string& cur_input,
        const std::string& full_input, int tab_count, const std::string& dir_path) {
    if (matches.empty()) { 
        std::cout << "\x07" << std::flush;
        return dir_path + cur_input;
    }
    std::string lcp = longest_common_prefix(matches);
    if (matches.size() == 1) {
        std::string suffix = lcp.substr(cur_input.size());
        std::string full = dir_path + lcp;
        char trailing_char = fs::is_directory(full) ? '/' : ' ';
        std::cout << suffix << trailing_char << std::flush;
        return full + trailing_char;
    }

    if (lcp.size() > cur_input.size()) {
        std::string suffix = lcp.substr(cur_input.size());
        std::cout << suffix << std::flush;
        return dir_path + lcp;
    }

    if (tab_count == 1) {
        std::cout << "\x07" << std::flush;
        return dir_path + cur_input;
    }

    std::sort(matches.begin(), matches.end());
    std::cout << "\n";
    for (size_t i = 0; i < matches.size(); i++) {
        if (i > 0) std::cout << "  ";
        std::string full = dir_path + matches[i];
        std::cout << matches[i] << (fs::is_directory(full) ? "/" : "");
    }
    std::cout << "\n$ " << full_input << std::flush;
    return cur_input;
}

std::string Util::longest_common_prefix(const std::vector<std::string>& matches) {
    if (matches.empty()) return "";
    if (matches.size() == 1) return matches[0];
    std::string lcp = "";
    for (size_t i = 0; i < matches[0].size(); i++) {
        char c = matches[0][i];

        for (size_t j = 1; j < matches.size(); j++) {
            if (i >= matches[j].size() || matches[j][i] != c) {
                return lcp;
            }
        }
        lcp += c;
    }
    return lcp;
}