#include "./headers/shell.h"
#include "./headers/parser.h"
#include "./headers/completer.h"
#include "./headers/filemanager.h"
#include "./headers/global.h"
#include "./headers/populater.h"
#include "./headers/raiiguard.h"
#include <unistd.h>
#include <iostream>
#include <filesystem>
#include <fcntl.h>

Shell::Shell() {
    register_builtins();
}

void Shell::register_builtins() {
    builtins["exit"] = [this](const auto& tokens) { return handle_exit(tokens); };
    builtins["echo"] = [this](const auto& tokens) { return handle_echo(tokens); };
    builtins["pwd"] = [this](const auto& tokens) {  return handle_pwd(tokens); };
    builtins["cd"] = [this](const auto& tokens) { return handle_cd(tokens);};
    builtins["type"] = [this](const auto& tokens) {return handle_type(tokens); };
    builtins["complete"] = [this](const auto& tokens) {return handle_complete(tokens); };
    builtins["jobs"] = [this](const auto& tokens) {  return handle_jobs(tokens); };
    builtins["history"] = [this](const auto& tokens) {  return handle_history(tokens); };
    builtins["declare"] = [this](const auto& tokens) {   return handle_declare(tokens); };
}

void Shell::setup_trie() {
    for (const auto& cmd : commands) {
        builtin_trie.insert(static_cast<std::string>(cmd));
    }
    Populater::populate_from_path(&builtin_trie);
    Populater::populate_files(&filename_trie);
}


void Shell::dispatch(std::string command) {
    shell_history.add(command); //add command to history
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

void Shell::run_chain(std::string& command) {
    auto segments = Parser::split_commands(command); //parse input based on command operators 
    int cur_status = 0;
    
    for (size_t i = 0; i < segments.size(); i++) {
        //determine the previous operator
        const std::string& last_op = (i == 0) ? "" : segments[i-1].op;
        
        //skip based on && and || logic
        if (last_op == "&&" && cur_status != 0) continue;
        if (last_op == "||" && cur_status == 0) continue;
        
        //check if this segment starts a pipeline by checking if op is |
        if (segments[i].op == "|") {
            //find the end of the pipeline
            size_t pipeline_end = i;
            while (pipeline_end < segments.size() && segments[pipeline_end].op == "|") {
                pipeline_end++;
            }
            pipeline_end++; //include the final command
            cur_status = execute_pipeline(segments, i, pipeline_end); //execute the entire pipeline
            i = pipeline_end - 1; //skip past the pipeline so we dont process them individually again
            continue;
        }
        
        //handle regular sequential command
        std::vector<std::string> tokens;
        Parser::parse(segments[i].command, tokens);
        Parser::variables_check(tokens, declare_builtin);
        
        std::string redirect_file = "";
        std::string redirect_stderr = "";
        int flags = O_TRUNC;
        std::vector<std::string> clean_tokens;
        
        Parser::parse_redirections(clean_tokens, tokens, redirect_file, redirect_stderr, flags);
        
        if (!clean_tokens.empty()) {      
            cur_status = execute_command(clean_tokens, redirect_file, redirect_stderr, flags);  //execute command with redirects
        }
    }
}

int Shell::execute_pipeline(const std::vector<CommandSegment>& segments, size_t start, size_t end) {
    std::vector<pid_t> pids; //stores pids for forked children
    std::vector<int> pipe_fds; //store file descriptors of created pipes
    
    //create all pipes first
    for (size_t i = start; i < end - 1; i++) {
        int pipefd[2]; //create file descriptors
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return 1;
        }
        pipe_fds.push_back(pipefd[0]); //adds read end
        pipe_fds.push_back(pipefd[1]); //adds write end
    }
    
    //fork and execute each command, loop through each command in the pipeline
    for (size_t i = start; i < end; i++) {
        pid_t pid = fork(); //fork and create child process
        
        if (pid == -1) {
            perror("fork");
            return 1;
        }
        
        if (pid == 0) {  //child process
            //redirect stdin from previous pipe if it is not the first
            if (i > start) {
                dup2(pipe_fds[2 * (i - start - 1)], STDIN_FILENO);
            }
            //redirect stdout to next pipe if not last
            if (i < end - 1) {
                dup2(pipe_fds[2 * (i - start) + 1], STDOUT_FILENO);
            }
            
            //close all pipe fds
            for (int fd : pipe_fds) {
                close(fd);
            }
            
            //parse command as normal
            std::vector<std::string> tokens;
            Parser::parse(segments[i].command, tokens);
            Parser::variables_check(tokens, declare_builtin);
            std::string redirect_file = "";
            std::string redirect_stderr = "";
            int flags = O_TRUNC;
            std::vector<std::string> clean_tokens;
            Parser::parse_redirections(clean_tokens, tokens, redirect_file, redirect_stderr, flags);
            
            if (clean_tokens.empty()) {
                exit(0); //if no valid command we exit process
            }
            
            std::string cmd = clean_tokens[0];
            
            //for builtins in pipelines
            if (builtins.find(cmd) != builtins.end()) {
                RAIIGuard guard(redirect_file, redirect_stderr, flags, &filename_trie);
                int status = builtins[cmd](clean_tokens);
                exit(status);
            }
            
            //for external commands directly call excvp so we dont create grandchild process
            std::vector<char*> argv;
            for (auto& token : clean_tokens) {
                argv.push_back(const_cast<char*>(token.c_str()));
            }
            argv.push_back(nullptr); //create arguments and push nullptr to end
            
            //handle output redirection if needed
            if (!redirect_file.empty()) {
                int fd = open(redirect_file.c_str(), O_WRONLY | O_CREAT | flags, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            
            execvp(clean_tokens[0].c_str(), argv.data());
            perror("execvp");  //only reached if execvp fails
            _exit(127);
        } else {  //parent
            pids.push_back(pid);
        }
    }
    
    //parent closes all pipes
    for (int fd : pipe_fds) {
        close(fd);
    }
    
    //wait for all children
    int last_status = 0;
    for (pid_t pid : pids) {
        int status = 0;
        waitpid(pid, &status, 0);
        last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    
    return last_status;
}

int Shell::execute_command(const std::vector<std::string>& clean_tokens,
                          const std::string& redirect_file,
                          const std::string& redirect_stderr,
                          int flags) {
    if (clean_tokens.empty()) return 0;
    
    std::string cmd = clean_tokens[0];
    
    //check if its a builtin
    if (builtins.find(cmd) != builtins.end()) {
        //use RAII guard to handle redirects
        RAIIGuard guard(redirect_file, redirect_stderr, flags, &filename_trie);
        return builtins[cmd](clean_tokens); //run function here
        //scope ends here and redirects are handled
    }
    
    //external command 
    return execute(Util::find_path(cmd), "", clean_tokens, redirect_file, redirect_stderr, flags);
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

int Shell::handle_exit(const std::vector<std::string>& tokens) {
    int code = 0;
    if (tokens.size() > 1) {
        try {
            code = std::stoi(tokens[1]);
        } catch (...) {
            code = 0;
        }
    }
    shell_history.close();
    std::exit(code);
    return 0; //never reached
}

int Shell::handle_echo(const std::vector<std::string>& tokens) {
    for (size_t i = 1; i < tokens.size(); i++) {
        if (i > 1) std::cout << " ";
        std::cout << tokens[i];
    }
    std::cout << std::endl;
    return 0;
}

int Shell::handle_pwd(const std::vector<std::string>& tokens) {
    std::cout << fs::current_path().string() << std::endl;
    return 0;
}

int Shell::handle_cd(const std::vector<std::string>& tokens) {
    cd_builtin.handle_cd(tokens.size() > 1 ? tokens[1] : "");
    return 0;
}

int Shell::handle_type(const std::vector<std::string>& tokens) {
    type_builtin.handle_type(tokens.size() > 1 ? tokens[1] : "", commands);
    return 0;
}

int Shell::handle_complete(const std::vector<std::string>& tokens) {
    complete_builtin.handle_complete_builtin(tokens);
    return 0;
}

int Shell::handle_jobs(const std::vector<std::string>& tokens) {
    jobs_builtin.handle_builtin();
    return 0;
}

int Shell::handle_history(const std::vector<std::string>& tokens) {
    shell_history.handle_builtin(tokens);
    return 0;
}

int Shell::handle_declare(const std::vector<std::string>& tokens) {
    declare_builtin.handle_builtin(tokens);
    return 0;
}