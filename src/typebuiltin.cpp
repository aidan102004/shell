#include "./builtins/typebuiltin.h"
#include "./headers/util.h"
#include <string>
#include <unordered_set>
#include <iostream>

void TypeBuiltin::handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins) {
    if (arg.empty()) return;
    if (builtins.find(arg) != builtins.end()) {
        std::cout << arg << " is a shell builtin" << std::endl;
        return;
    }
    std::string found_path = Util::find_path(arg);
    if (!found_path.empty()) {
        std::cout << arg << " is " << found_path << std::endl;
    } else {
        std::cout << arg << ": not found" << std::endl;
    }
}