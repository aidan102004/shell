#include "./headers/shell.h"
#include "./headers/parser.h"
#include "./headers/completer.h"
#include "./headers/filemanager.h"
#include "./headers/global.h"
#include "./headers/populater.h"
#include <unistd.h>
#include <iostream>
#include <filesystem>
#include <fcntl.h>

void Shell::setup_trie() {
    for (const auto& cmd : commands) {
        builtin_trie.insert(static_cast<std::string>(cmd));
    }
    Populater::populate_from_path(&builtin_trie);
    Populater::populate_files(&filename_trie);
}


void Shell::dispatch(std::string command) {
    std::string full_cmd = "";
    while (!command.empty() && command.back() == ' ') command.pop_back(); //strip whitespace

    bool whole_chain_bg = false; 
    if (!command.empty() && command.back() == '&') {
        if (command.size() < 2 || command[command.size()-2] != '&') { //if this is the only & meaning bg
            whole_chain_bg = true;
            full_cmd = command;
            command.pop_back(); //remove &
            while (!command.empty() && command.back() == ' ') command.pop_back(); //strip whitespace again
        }
    }

    if (whole_chain_bg) {
        pid_t pid = fork(); //complete whole command in its own bg
        if (pid == 0) {
            int dev_null = open("/dev/null", O_RDONLY);
            if (dev_null >= 0) {
                dup2(dev_null, STDIN_FILENO);
                close(dev_null);
            }
            run_chain(command);  
            _exit(0);
        }
        //add new job
        jobs_builtin.add_job(pid, full_cmd);
    } else {
        run_chain(command);
    }
}

void Shell::run_chain(std::string& command) 
{
    std::vector<std::string> tokens;
    auto segments = Parser::split_commands(command);
    int cur_process_status = 0;
    for (size_t i = 0; i < segments.size(); i++) {
        //check for pipe | operator 
        //if (segments[i].op == "|") 

        const std::string& last_op = (i==0) ? "" : segments[i-1].op;
        
        if (last_op == "&&" && cur_process_status != 0) continue;
        if (last_op == "||" && cur_process_status == 0) continue;

        std::vector<std::string> tokens;
        Parser::parse(segments[i].command, tokens);
        Parser::variables_check(tokens, declare_builtin);
        std::string redirect_file = ""; 
        int FLAG_CONST = O_TRUNC; //default for file redirection
        std::string redirect_stderr = "";
        std::vector<std::string> clean_tokens;
        Parser::parse_redirections(clean_tokens, tokens, redirect_file, redirect_stderr, FLAG_CONST);
        
        if (clean_tokens.empty()) continue;
        bool is_bg = clean_tokens.back() == "&";
        if (is_bg) clean_tokens.pop_back();

        std::string cmd = clean_tokens[0];

        int status = 0;
        if (cmd == "exit") {
            int code = 0;
            if (clean_tokens.size() > 1) {
                try {
                    code = std::stoi(clean_tokens[1]);
                } catch (...) {
                    code = 0; 
                }
            }
            shell_history.close();
            std::exit(code);
        } else if (cmd == "type") {
            int saved_out = FileManager::redirect_fd(STDOUT_FILENO, FLAG_CONST, redirect_file);
            int saved_err = FileManager::redirect_fd(STDERR_FILENO, FLAG_CONST, redirect_stderr);
            type_builtin.handle_type(clean_tokens.size() > 1 ? clean_tokens[1] : "", commands);
            FileManager::restore_fd(STDOUT_FILENO, saved_out);
            FileManager::restore_fd(STDERR_FILENO, saved_err);
            status = 0;
        } else if (cmd == "echo") {
            int saved_out = FileManager::redirect_fd(STDOUT_FILENO, FLAG_CONST, redirect_file);
            int saved_err = FileManager::redirect_fd(STDERR_FILENO, FLAG_CONST, redirect_stderr);
            for (size_t i = 1; i < clean_tokens.size(); i++) {
                if (i > 1) std::cout << " ";
                std::cout << clean_tokens[i];
            }
            std::cout << std::endl;
            FileManager::restore_fd(STDOUT_FILENO, saved_out);
            FileManager::restore_fd(STDERR_FILENO, saved_err);
            status = 0;
        } else if (cmd == "pwd") {
            int saved_out = FileManager::redirect_fd(STDOUT_FILENO, FLAG_CONST, redirect_file);
            int saved_err = FileManager::redirect_fd(STDERR_FILENO, FLAG_CONST, redirect_stderr);
            std::cout << fs::current_path().string() << std::endl;
            FileManager::restore_fd(STDOUT_FILENO, saved_out);
            FileManager::restore_fd(STDERR_FILENO, saved_err);
            status = 0;
        } else if (cmd == "cd") {
            cd_builtin.handle_cd(clean_tokens.size() > 1 ? clean_tokens[1] : "");
            status = 0;
        } else if (cmd == "complete" ){
            complete_builtin.handle_complete_builtin(clean_tokens);
            status = 0;
        } else if (cmd == "jobs") {
            jobs_builtin.handle_builtin();
            //handle_jobs_builtin();
            status = 0;
        } else if (cmd == "history") {
            shell_history.handle_builtin(clean_tokens);
        } else if (cmd == "declare") {
            declare_builtin.handle_builtin(clean_tokens);
        } else {
            status = execute(Util::find_path(cmd), command, clean_tokens, redirect_file, redirect_stderr, FLAG_CONST);
        }  
        cur_process_status = status; 
    }
}

int Shell::execute(const std::string& exe_path, const std::string& command,
             const std::vector<std::string>& tokens, const std::string& redirect_file, const std::string& redirect_stderr, int FLAG_CONST) {
    if (exe_path.empty()) {
        std::cerr << command << ": command not found" << std::endl;
        return 127;
    }

    std::vector<char*> argv;
    for (auto& t : tokens) {
        argv.push_back(const_cast<char*>(t.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (!redirect_file.empty()) {
            int fd = open(redirect_file.c_str(), O_WRONLY | O_CREAT | FLAG_CONST, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (!redirect_stderr.empty()) {           
            int fd = open(redirect_stderr.c_str(), O_WRONLY | O_CREAT | FLAG_CONST, 0644);
            dup2(fd, STDERR_FILENO);              
            close(fd);
        }
        execvp(exe_path.c_str(), argv.data());
        perror("execvp");
        _exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

std::string Shell::read_input() {
    std::string loc_buffer;
    size_t cursor_pos = 0;
    char c;
    int tab_count = 0;
    while (read(STDIN_FILENO, &c, 1) > 0) { //while input reading doesnt return 0 bytes
        if (c == '\n') { std::cout << '\n'; break; } //enter

        //arrow key handling for history 
        else if (c == '\x1b') {  // ESC character
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) > 0 && seq[0] == '[') {
                if (read(STDIN_FILENO, &seq[1], 1) > 0) {
                    if (seq[1] == 'A') {
                        std::string prev = shell_history.previous();
                        loc_buffer = prev;
                        cursor_pos = loc_buffer.size();
                    } 
                    else if (seq[1] == 'B') {
                        std::string next = shell_history.next();
                        loc_buffer = next;
                        cursor_pos = loc_buffer.size();
                    } 
                    else if (seq[1] == 'C') {
                        // right arrow
                        if (cursor_pos < loc_buffer.size()) {
                            std::cout << "\x1b[C";
                            cursor_pos++;
                        }
                    } 
                    else if (seq[1] == 'D') {
                        // left arrow
                        if (cursor_pos > 0) {
                            std::cout << "\x1b[D";
                            cursor_pos--;
                        }
                    }
                }
            }
        }
        else if (c == '\t')
        { 
            std::string s = "";
            if (loc_buffer.find(' ') != std::string::npos) {
                size_t pos = loc_buffer.rfind(' ');
                std::string arg = loc_buffer.substr(pos + 1);
                std::string command_name = loc_buffer.substr(0, loc_buffer.find(' '));
                bool found = Completer::find_path(command_name);
                if (found) {
                    //handle case where the is custom complete specification for a cmd
                    tab_count++;
                    s = complete_builtin.run_completer(Completer::get_path(command_name), command_name, arg, loc_buffer, tab_count); //run completer
                } else if (arg.rfind('/') != std::string::npos) {
                    tab_count++;
                    size_t slash_pos = arg.rfind('/'); //position of the slash
                    s = Completer::path_completion(arg, loc_buffer, slash_pos, tab_count); // this method returns the string of the completed path
                } else {
                    tab_count++;
                    s = Completer::completion(filename_trie, arg, loc_buffer, tab_count);
                }
                s = loc_buffer.substr(0, pos) + " " + s;
                
            } else {
                tab_count++; 
                s = Completer::completion(builtin_trie, loc_buffer, loc_buffer, tab_count); //on tab press check trie
            }
            if (s != loc_buffer) {
                loc_buffer = s;
                cursor_pos = loc_buffer.size();
                tab_count = 0;
            }
        }
        else if (c == 127) {
            tab_count = 0;
            if (!loc_buffer.empty()) {
                loc_buffer.pop_back();
                cursor_pos--;
                std::cout << "\b \b" << std::flush; //backspace
            }
        } else {
            tab_count = 0;
            loc_buffer.insert(cursor_pos, 1, c);
            cursor_pos++;
            std::cout << c;

            {
                std::lock_guard<std::mutex> lock(i_mutex);
                current_input = loc_buffer; //update our global input var
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(i_mutex);
        current_input.clear();
    }
    return loc_buffer;
}
