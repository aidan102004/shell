#pragma once

#include <string>
#include <vector>
#include "../builtins/shellhistory.h"
#include "../builtins/declarebuiltin.h"
#include "../builtins/jobsbuiltin.h"
#include "../builtins/completebuiltin.h"
#include "../builtins/typebuiltin.h"
#include "../builtins/cdbuiltin.h"
#include "trie.h"

class Shell {
private:
    // Builtin commands list
    std::unordered_set<std::string> commands = {
        "echo", "exit", "type", "pwd", "cd", "complete", "jobs", "history", "declare"
    };
    ShellHistory shell_history;
    DeclareBuiltin declare_builtin;
    JobsBuiltin jobs_builtin;
    CompleteBuiltin complete_builtin;
    TypeBuiltin type_builtin;
    CdBuiltin cd_builtin;

    Trie builtin_trie;
    Trie filename_trie; 
public:
    void setup_trie();
    void dispatch(std::string command);
    void run_chain(std::string& command);
    int execute(const std::string& exe_path, const std::string& command, const std::vector<std::string>& tokens, const std::string& redirect_file, const std::string& redirect_stderr, int FLAG_CONST);
    std::string read_input();
};