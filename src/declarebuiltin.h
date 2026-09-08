#pragma once

#include <any>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>  
#include <functional>     

class DeclareBuiltin {
    using FlagHandler = std::function<void(const std::string&)>; //alias for function pointer
    static inline std::map<std::string, FlagHandler> flag_handlers;//create map of function pointers
    std::unordered_map<std::string , std::any> variables;

    public:

    DeclareBuiltin();
    void register_handlers();
    void handle_builtin(const std::vector<std::string>& tokens);
    void handle_print(const std::string& arg);
    void add_variable(const std::string& arg);
}; 
