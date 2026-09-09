#include "declarebuiltin.h"

#include <vector>
#include <string>
#include <map>
#include <unordered_map>  
#include <functional>     
#include <any>
#include <sstream>
#include <iostream>

DeclareBuiltin::DeclareBuiltin() {
    static bool initialised = false;
    if (!initialised) {
        register_handlers();
        initialised = true;
    }
}

void DeclareBuiltin::register_handlers() {
    flag_handlers["-p"] = [this](const std::string& arg, bool ro) {handle_print(arg, ro);};
    flag_handlers["-i"] = [this](const std::string& arg, bool ro) {handle_int(arg, ro);};
    flag_handlers["-a"] = [this](const std::string& arg, bool ro) {handle_array(arg, ro);}; 
    //flag_handlers["-A"] = [this](const std::string& arg) {handle_print(arg);}; map
    //flag_handlers["-x"] = [this](const std::string& arg) {handle_print(arg);}; export
}

void DeclareBuiltin::handle_builtin(const std::vector<std::string>& tokens) {
    //bounds check
    if (tokens.size() < 2) {
        std::cerr << "declare: missing arguments" << std::endl;
        return;
    }
    
    std::string flag = tokens[1]; //set flag to first token
    bool ro = false;
    int token_index = 1; //this iterates what we pass into addvariable in the case that the flag is just the var declaration
    
    //check for readonly flag
    size_t r_pos = flag.find(RO_FLAG);
    if (r_pos != std::string::npos && r_pos > 0 && flag[r_pos - 1] == '-') { //check if we find it and confirm its the flag by checking if the prev char is -
        ro = true; 
        if (flag.length() > 2) { //checking if flag is -r or something like -ra -ri
            flag = '-' + flag.substr(r_pos + 1);
        } else {
            flag = "";
        }
        token_index += 1; //iterate this incase it was just -r
    }
    
    std::string arg = (tokens.size() > 2) ? tokens[2] : ""; //set arg
    auto it = flag_handlers.find(flag); //search through map of func ptr to run respective function
    if (it != flag_handlers.end()) {
        it->second(arg, ro); //we run the function here 
        return;
    } else {
        if (tokens.size() > token_index) {
            add_variable(tokens[token_index], ro, "string"); //default type is string
        }
        return;
    }
}

void DeclareBuiltin::add_variable(const std::string& arg, bool ro, const std::string& type) {
    if (arg.empty() || std::isdigit(arg[0])) { //check first letter of var name isnt a digit
        std::cerr << "declare: " << arg << ": not a valid identifier" << std::endl;
        return; 
    }
    
    size_t pos = arg.find("="); //find equal sign and split
    if (pos == std::string::npos) {
        std::cerr << "declare: " << arg << ": is not a valid argument" << std::endl;
        return;
    }
    
    //search through map for variable name
    auto it = variables.find(arg.substr(0, pos));
    
    //set type 
    std::any value;
    if (type == "integer") {
        try {
            value = std::stoi(arg.substr(pos + 1)); //int
        } catch (...) {
            std::cerr << "declare: invalid integer value" << std::endl;
            return;
        }
    } 
    else if (type == "array") {
        value = parseArray(arg.substr(pos + 1)); //vector
    } else {
        value = arg.substr(pos + 1); //string
    }

    if (it == variables.end()) {
        variables[arg.substr(0, pos)] = {value, type, ro}; //if variable doesnt exist in map, add it
    } 
    else if (it->second.read_only) {
        std::cerr << "declare: cannot modify readonly variable" << std::endl; //if read only dont modify
    }
    else {
        variables[arg.substr(0, pos)] = {value, type, ro}; //not readonly and in map, so update it
    }
}

void DeclareBuiltin::handle_print(const std::string& arg, bool ro) {
    auto it = variables.find(arg);
    if (it != variables.end()) {
        std::cout << "declare -- " << arg << "=" << anyToString(it->second.value) << std::endl;
    } else {
        std::cerr << "declare: " << arg << ": not found" << std::endl;
    }
}

void DeclareBuiltin::handle_int(const std::string& arg, bool ro) {
    add_variable(arg, ro, "integer");
}

void DeclareBuiltin::handle_array(const std::string& arg, bool ro) {
    add_variable(arg, ro, "array");
}

std::vector<std::any> DeclareBuiltin::parseArray(const std::string& value) {
    std::vector<std::any> result;
    if (value.empty() || value[0] != '(' || value[value.size() - 1] != ')') {
        return result;
    }
    std::string content = value.substr(1, value.size() - 2);
    std::istringstream iss(content);
    std::string word;
    while (iss >> word) {
        try {
            result.push_back(std::stoi(word)); //try int first
        } catch (...) {
            result.push_back(word); //just push as string
        }
    }
    return result;
}

std::string DeclareBuiltin::anyToString(const std::any& value) {
    if (value.type() == typeid(int)) {
        return std::to_string(std::any_cast<int>(value));
    }
    else if (value.type() == typeid(std::string)) {
        return std::any_cast<std::string>(value);
    }
    else if (value.type() == typeid(std::vector<std::any>)) {
        auto arr = std::any_cast<std::vector<std::any>>(value);
        std::string result = "[";
        for (size_t i = 0; i < arr.size(); ++i) {
            result += anyToString(arr[i]);
            if (i < arr.size() - 1) result += ", ";
        }
        result += "]";
        return result;
    }
    return "unknown";
}

std::optional<std::any> DeclareBuiltin::get_var(const std::string& key) {
    auto it = variables.find(key);
    if (it != variables.end()) {
        return it->second.value;
    } else {
        return std::nullopt;
    }
}