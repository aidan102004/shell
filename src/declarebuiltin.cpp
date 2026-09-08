#include "declarebuiltin.h"

#include <vector>
#include <string>
#include <map>
#include <unordered_map>  
#include <functional>     
#include <any>
#include <iostream>

DeclareBuiltin::DeclareBuiltin() {
    static bool initialised = false;
    if (!initialised) {
        register_handlers();
        initialised = true;
    }
}

void DeclareBuiltin::register_handlers() {
    flag_handlers["-p"] = [this](const std::string& arg) {handle_print(arg);};
}

void DeclareBuiltin::handle_builtin(const std::vector<std::string>& tokens) {
    std::string flag = tokens[1];
    std::string arg = (tokens.size() > 2) ? tokens[2] : ""; //passes empty string in case there is no arg, handle this in function
    auto it = flag_handlers.find(flag);
    if (it != flag_handlers.end()) {
        it->second(arg); //run function
        return;
    } else {
        add_variable(tokens[1]);
        return;
    }
    std::cerr << "Error: " << flag << " unknown flag\n";
}

void DeclareBuiltin::handle_print(const std::string& arg) {
    auto it = variables.find(arg);
    if (it != variables.end()) {
        std::cout << "declare -- " << arg << "=" << std::any_cast<std::string>(variables[arg]) << std::endl;
    } else {
        std::cout << "declare: " << arg << ": not found" << std::endl;
    }
}

void DeclareBuiltin::add_variable(const std::string& arg) {
    if (std::isdigit(arg[0])) std::cout << "declare: " << arg << ": not a valid identifier" << std::endl;
    size_t pos = arg.find("=");
    if (pos == std::string::npos) {
        std::cout << "declare: " << arg << ": is not a valid argument" << std::endl;
        return;
    }
    std::string name = arg.substr(0, pos);
    std::any var = arg.substr(pos + 1);
    variables[name] = var;
}