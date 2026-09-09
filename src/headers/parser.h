#pragma once

#include <string>
#include <vector>
#include "command.h"
#include "../builtins/declarebuiltin.h"

class Parser {
public:
    static void parse(const std::string& command, std::vector<std::string>& tokens);
    static void variables_check(std::vector<std::string>& tokens, DeclareBuiltin& declare_builtin); 
    static std::vector<CommandSegment> split_commands(const std::string& command);
    static std::vector<std::string> parse_redirections(std::vector<std::string>& clean_tokens, std::vector<std::string>& tokens, std::string& redirect_file, std::string& redirect_stderr, int& FLAG_CONST);
};