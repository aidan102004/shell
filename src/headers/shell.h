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
#include "command.h"

using BuiltinHandler = std::function<int(const std::vector<std::string>&)>;

class Shell {
private:
    // Builtin commands list
    std::unordered_map<std::string, BuiltinHandler> builtins;
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

    void register_builtins();
    int execute_command(const std::vector<std::string>& clean_tokens, const std::string& redirect_file, const std::string& redirect_stderr, int flags);
    int execute_pipeline(const std::vector<CommandSegment>& segments, size_t start, size_t end);
    
    //builtin handlers
    int handle_exit(const std::vector<std::string>& tokens);
    int handle_echo(const std::vector<std::string>& tokens);
    int handle_pwd(const std::vector<std::string>& tokens);
    int handle_cd(const std::vector<std::string>& tokens);
    int handle_type(const std::vector<std::string>& tokens);
    int handle_complete(const std::vector<std::string>& tokens);
    int handle_jobs(const std::vector<std::string>& tokens);
    int handle_history(const std::vector<std::string>& tokens);
    int handle_declare(const std::vector<std::string>& tokens);
public:
    Shell();
    void setup_trie();
    void dispatch(std::string command);
    void run_chain(std::string& command);
    int execute(const std::string& exe_path, const std::string& command, const std::vector<std::string>& tokens, const std::string& redirect_file, const std::string& redirect_stderr, int FLAG_CONST);
    std::string read_input();
};