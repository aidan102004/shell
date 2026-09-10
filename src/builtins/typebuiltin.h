#pragma once
#include <string>
#include <unordered_set>

class TypeBuiltin {
public:
    void handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins);
};