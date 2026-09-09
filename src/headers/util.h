#pragma once

#include <string>
#include <vector>

class Util {
public:
    static std::string matches_helper(std::vector<std::string>& matches, const std::string& cur_input,
        const std::string& full_input, int tab_count, const std::string& dir_path = "");
    static std::string longest_common_prefix(const std::vector<std::string>& matches);
};
