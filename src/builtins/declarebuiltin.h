#pragma once

#include <any>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>  
#include <functional>     

class DeclareBuiltin {
private:
    struct VarMetaData {
        std::any value;
        std::string type;
        bool read_only = false;
    };
    using FlagHandler = std::function<void(const std::string&, bool ro)>; //alias for function pointer
    static inline std::map<std::string, FlagHandler> flag_handlers;//create map of function pointers
    std::unordered_map<std::string , VarMetaData> variables;
    static const char RO_FLAG = 'r';

public:
    DeclareBuiltin();
    void register_handlers();
    std::optional<std::any> get_var(const std::string& key);
    void handle_builtin(const std::vector<std::string>& tokens);
    void handle_print(const std::string& arg, bool ro) ;
    void add_variable(const std::string& arg, bool ro, const std::string& type);
    void handle_int(const std::string& arg, bool ro);
    void handle_array(const std::string& arg, bool ro);
    std::vector<std::any> parseArray(const std::string& value);
    std::string anyToString(const std::any& value);

}; 
