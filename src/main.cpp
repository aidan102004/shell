#include <iostream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <list>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include "trie.h"
#include "job.h"
#include "command.h"

namespace fs = std::__fs::filesystem;

// Forward declarations
void handle_jobs_builtin();
void add_job(int j_num, pid_t pid, std::string full_cmd);
void dispatch(std::string command);
void run_chain(std::string& command);
std::vector<CommandSegment> split_commands(const std::string& command);
std::vector<std::string> parse_redirections(std::vector<std::string>& clean_tokens, std::vector<std::string>& tokens, std::string& redirect_file, std::string& redirect_stderr, int& FLAG_CONST);
pid_t bg_job(const std::string& exe_path, std::vector<std::string>& tokens);
void handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins);
std::string find_path(const std::string& arg);
int execute(const std::string& exe_path, const std::string& command, const std::vector<std::string>& tokens, const std::string& redirect_file, const std::string& redirect_stderr, int FLAG_CONST);
void handle_cd(const std::string& arg);
void handle_complete_builtin(std::vector<std::string>& args);
std::string run_completer(const fs::path& script, const std::string& command, const std::string& curr_word, const std::string& full_input, int tab_count);
std::vector<std::string> execute_completer(const fs::path& script, const std::string& command, const std::string& curr_word, const std::string& prev_word, const std::string& f_in);
std::string read_input();
std::string completion(Trie& trie, std::string cur_input, const std::string& full_line, int tab_count);
void parse(const std::string& command, std::vector<std::string>& tokens);
void populate_from_path();
std::string longest_common_prefix(const std::vector<std::string>& matches);
void populate_files();
std::string path_completion(const std::string& s, const std::string& full_line, size_t s_pos, int tab_count);
std::string matches_helper(std::vector<std::string>& matches, const std::string& cur_input, const std::string& full_input, int tab_count, const std::string& dir_path ="");

// Builtin commands list
std::unordered_set<std::string> commands = {
    "echo", "exit", "type", "pwd", "cd", "complete", "jobs"
};

std::unordered_map<std::string, fs::directory_entry> complete_paths;
std::map<int, Job> jobs;
std::mutex j_mutex;
std::pair<int,int> cur_prev_jobs = {-1, -1};


Trie builtin_trie;
Trie filename_trie;

std::string current_input;
std::mutex i_mutex;
struct termios original_termios;
int redirect_fd(int fd_num, int FLAG_CONST, const std::string& path) {
    if (path.empty()) return -1;
    int saved = dup(fd_num);
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | FLAG_CONST, 0644);
    if (fd < 0) {
        perror("open");
        close(saved);
        return -1;
    }
    dup2(fd, fd_num);
    close(fd);
    return saved;
}

// Restores stdout from a saved fd produced by redirect_stdout
void restore_fd(int fd_num, int saved_fd) {
    if (saved_fd == -1) return;
    dup2(saved_fd, fd_num);
    close(saved_fd);
}

void disable_raw() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios); //restores terminal to canonical settings 
}
void enable_raw() {
    tcgetattr(STDIN_FILENO, &original_termios); //saves current terminal settings 
    atexit(disable_raw); //when program exits run method
    struct termios raw = original_termios; //create temp termios obejct
    raw.c_lflag &= ~(ECHO | ICANON); //turn off echo and icanon flag, doesnt display keypresses, turns off line buffered input
    tcsetattr(STDIN_FILENO, TCSANOW, &raw); //applies settings
}

int main() {
    std::cout << std::unitbuf; //flush cout and cerr after every output
    std::cerr << std::unitbuf;
    std::string command;
    //populate trie
    for (const auto& cmd : commands) {
        builtin_trie.insert(static_cast<std::string>(cmd));
    }
    populate_from_path();
    populate_files();
    while (true) {
        std::cout << "$ ";
        enable_raw(); //swap from canonical to raw
        std::string command = read_input(); //read input
        if (command.empty()) continue;
        dispatch(command);
    }
}

void dispatch(std::string command) {
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
        add_job(jobs.size() + 1, pid, full_cmd);
    } else {
        run_chain(command);
    }
}

void handle_jobs_builtin() 
{
    const int pad_const = 24; 
    std::cout << jobs.size() << std::endl;
    for (const auto& [order, job] : jobs) 
    {
        int pad_delta = pad_const - std::to_string(abs(static_cast<int>(job.process_id))).size();
        std::string status = (job.status == true) ? "Running" : "Done";
        char marker = ' ';
        if (order >= jobs.size() || jobs.size() == 1) {
            marker = '+';
        } else if (order == jobs.size() - 1) {
            marker = '-';
        }
        std::cout << "[" << job.j_num << "]" << marker << " " << status << std::setw(pad_delta) << job.command_str << std::endl;;
    }
}

void run_chain(std::string& command) 
{
    std::vector<std::string> tokens;
    auto segments = split_commands(command);
    int cur_process_status = 0;
    for (size_t i = 0; i < segments.size(); i++) {
        const std::string& last_op = (i==0) ? "" : segments[i-1].op;
        
        if (last_op == "&&" && cur_process_status != 0) continue;
        if (last_op == "||" && cur_process_status == 0) continue;

        std::vector<std::string> tokens;
        parse(segments[i].command, tokens);
        std::string redirect_file = ""; 
        int FLAG_CONST = O_TRUNC; //default for file redirection
        std::string redirect_stderr = "";
        std::vector<std::string> clean_tokens;
        parse_redirections(clean_tokens, tokens, redirect_file, redirect_stderr, FLAG_CONST);
        
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
            std::exit(code);
        } else if (cmd == "type") {
            int saved_out = redirect_fd(STDOUT_FILENO, FLAG_CONST, redirect_file);
            int saved_err = redirect_fd(STDERR_FILENO, FLAG_CONST, redirect_stderr);
            handle_type(clean_tokens.size() > 1 ? clean_tokens[1] : "", commands);
            restore_fd(STDOUT_FILENO, saved_out);
            restore_fd(STDERR_FILENO, saved_err);
            status = 0;
        } else if (cmd == "echo") {
            int saved_out = redirect_fd(STDOUT_FILENO, FLAG_CONST, redirect_file);
            int saved_err = redirect_fd(STDERR_FILENO, FLAG_CONST, redirect_stderr);
            for (size_t i = 1; i < clean_tokens.size(); i++) {
                if (i > 1) std::cout << " ";
                std::cout << clean_tokens[i];
            }
            std::cout << std::endl;
            restore_fd(STDOUT_FILENO, saved_out);
            restore_fd(STDERR_FILENO, saved_err);
            status = 0;
        } else if (cmd == "pwd") {
            int saved_out = redirect_fd(STDOUT_FILENO, FLAG_CONST, redirect_file);
            int saved_err = redirect_fd(STDERR_FILENO, FLAG_CONST, redirect_stderr);
            std::cout << fs::current_path().string() << std::endl;
            restore_fd(STDOUT_FILENO, saved_out);
            restore_fd(STDERR_FILENO, saved_err);
            status = 0;
        } else if (cmd == "cd") {
            handle_cd(clean_tokens.size() > 1 ? clean_tokens[1] : "");
            status = 0;
        } else if (cmd == "complete" ){
            handle_complete_builtin(clean_tokens);
            status = 0;
        } else if (cmd == "jobs") {
            handle_jobs_builtin();
            status = 0;
        } else {
            if (is_bg) {
                pid_t pid = bg_job(find_path(cmd), clean_tokens);
                if (pid > 0) {
                    int j_num = jobs.size() + 1;
                    jobs[j_num] = {j_num, pid, cmd, true};
                    std::cout << "[" << j_num << "] " << pid << std::endl;
                    status = 0;
                } else {
                    status = 1;
                }
            } else {
            status = execute(find_path(cmd), command, clean_tokens, redirect_file, redirect_stderr, FLAG_CONST);
            } 
        }  
        cur_process_status = status; 
    }
}

std::vector<CommandSegment> split_commands(const std::string& command) {
    std::vector<CommandSegment> segments;
    std::string cur = "";
    bool iq = false;
    bool idq = false;
    for (size_t i = 0; i < command.size(); i++) {
        char c = command[i];
        if (c == '\\' && !iq && !idq) {
            if (i + 1 < command.size()) {
                cur += c;
            }
        } else if (c == '\\' && idq) {          // backslash inside double quotes
            if (i + 1 < command.size()) {
                char next = command[i + 1];
                if (next == '"' || next == '\\') {
                    cur += next;
                } else {
                    cur += c;
                }
            }  
        } else if (c == '\"' && !idq && !iq) {
            idq = true;
        } else if (c == '\'' && !iq && !idq) {
            iq = true;
        } else if (c == '\'' && iq) {
            iq = false;
        } else if (c == '\"' && idq) {
            idq = false;
        } else if (c == '&' && i + 1 < command.size() && command[i+1] == '&' && !iq && !idq) {
            segments.push_back({cur, "&&"});
            cur.clear();
            i++;
            continue;
        } else if (c == '|' && i + 1 < command.size() && command[i+1] == '|' && !iq && !idq) {
            segments.push_back({cur, "||"});
            cur.clear();
            i++;
            continue;
        } else if (c == ';' && i + 1 < command.size() && !iq && !idq) {
            segments.push_back({cur, ";"});
            cur.clear();
            i++;
            continue;
        } else {
            cur += c;
        }
    }

    if (!cur.empty()) segments.push_back({cur, ""});
    return segments;
}

void add_job(int j_num, pid_t pid, std::string full_cmd) 
{
    //lambda func
    auto monitor_func = [](Job& job) {
        int status;
        pid_t res = waitpid(job.process_id, &status, 0);
        if (res == job.process_id) {
            std::string snapshot;
            {
                std::lock_guard<std::mutex> lock(i_mutex);
                snapshot = current_input;
            }
            int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
            std::cout << "\r\033[K";
            if (exit_code == 0) std::cout << "[" << job.j_num << "]+  Done    " << job.command_str << std::endl;
            else std::cout << "[" << job.j_num << "]+  Exit " << exit_code << "  " << job.command_str << std::endl;
            std::cout << "$ " << snapshot << std::flush; 
            job.status = false;
            {
                std::lock_guard<std::mutex> lock(j_mutex);
                jobs.erase(job.j_num);
            }
        }
    };
    //create job
    {
        std::lock_guard<std::mutex> lock(j_mutex);
        jobs[j_num] = {j_num, pid, full_cmd, true};
    }
    std::cout << "[" << j_num << "] " << pid << std::endl;
    std::thread t(monitor_func, std::ref(jobs[j_num]));
    t.detach();
}
std::vector<std::string> parse_redirections(std::vector<std::string>& clean_tokens, std::vector<std::string>& tokens, std::string& redirect_file, std::string& redirect_stderr, int& FLAG_CONST)
{
    for (size_t i = 0; i < tokens.size(); i++) {
        if ((tokens[i] == ">" || tokens[i] == "1>") && i + 1 < tokens.size()) {
            redirect_file = tokens[i + 1];
            i++; // skip filename token too
        } else if ((tokens[i] == ">>" || tokens[i] == "1>>") && i + 1 < tokens.size()) {
            redirect_file = tokens[i + 1];
            FLAG_CONST = O_APPEND; //change flag for append
            i++; 
        }
        else if (tokens[i] == "2>" && i + 1 <tokens.size()) {
            redirect_stderr = tokens[i + 1];
            i++;
        } else if (tokens[i] == "2>>" && i + 1 <tokens.size()) {
            redirect_stderr = tokens[i + 1];
            FLAG_CONST = O_APPEND; //change flag for append
            i++;
        } else {
            clean_tokens.push_back(tokens[i]);
        }
    }
    return clean_tokens;
}


pid_t bg_job(const std::string& exe_path, std::vector<std::string>& tokens){
    std::vector<char*> argv;
    for (auto& t : tokens) {
        argv.push_back(const_cast<char*>(t.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return pid;
    }

    if (pid == 0) {
        int dev_null = open("/dev/null", O_RDONLY);
        if (dev_null >= 0) {
            dup2(dev_null, STDIN_FILENO);
            close(dev_null);
        }
        execv(exe_path.c_str(), argv.data());
        perror("execv");
        _exit(1);
    }

    return pid;

}

void handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins) {
    if (arg.empty()) return;
    if (builtins.find(arg) != builtins.end()) {
        std::cout << arg << " is a shell builtin" << std::endl;
        return;
    }
    std::string found_path = find_path(arg);
    if (!found_path.empty()) {
        std::cout << arg << " is " << found_path << std::endl;
    } else {
        std::cout << arg << ": not found" << std::endl;
    }
}

void handle_cd(const std::string& arg) {
    std::string target = arg;
    const char* path;
    if (target.empty() || target == "~") {
        path = std::getenv("HOME");
    } else {
        path = target.c_str();
    }
    if (chdir(path) != 0) {
        std::cerr << "cd: " << path << ": No such file or directory" << std::endl;
    }
}

void handle_complete_builtin(std::vector<std::string>& args) {
    std::string flag = args[1];
    fs::directory_entry path(args[2]);
    std::string cmd = args.back();
    if (flag == "-C") {
        complete_paths[cmd] = path;
    } else if (flag == "-p") {
        auto it = complete_paths.find(cmd);
        if (it != complete_paths.end()) {
            std::cout << "complete -C '" << it->second.path().string() << "' " << cmd << std::endl; 
        } else {
            std::cout << "complete: " << cmd << ": no completion specification" << std::endl;
        }
    } else if (flag == "-r") {
        complete_paths.erase(cmd);
    }
}
std::string run_completer(const fs::path& script, const std::string& command, const std::string& curr_word, const std::string& full_input, int tab_count) 
{
    std::string before_curr = full_input.substr(0, full_input.rfind(' '));
    size_t second_last_space = before_curr.rfind(' ');
    std::string prev_word;

    if (curr_word.empty()) {
        prev_word = "";
    } else if (second_last_space != std::string::npos) {
        prev_word = before_curr.substr(second_last_space + 1);
    } else {
        prev_word = before_curr;   
    }
    std::vector<std::string> candidates = execute_completer(script, command, curr_word, prev_word, full_input);
    return matches_helper(candidates, curr_word, full_input, tab_count);
}
std::vector<std::string> execute_completer(const fs::path& script, const std::string& command, const std::string& curr_word, const std::string& prev_word, const std::string& f_in) {
    int fds[2], nbytes, status; //create a 2 element array that pipe() will fill in with read and write, nbytes will hold read value
    std::vector<std::string> res;

    if (pipe(fds) == -1) { //create pipeline
        perror("pipe");
        res.push_back(curr_word);
        return res; //return unchanged arg if there is an error
    }
    pid_t pid = fork(); //duplicates the entire current process 
    if (pid == -1) {
        perror("fork");
        res.push_back(curr_word);
        return res;
    }
    if (pid == 0) { //child process
        setenv("COMP_LINE", f_in.c_str(), 1);
        setenv("COMP_POINT", std::to_string(f_in.size()).c_str(), 1);
        close(fds[0]); //child process doesnt need to read
        dup2(fds[1], STDOUT_FILENO); //redirect stdout to fds[1]
        close(fds[1]); //close write
        execl(script.c_str(), script.c_str(), command.c_str(), curr_word.c_str(), prev_word.c_str(), nullptr); //replace child process with the script
        perror("execvp");
        _exit(1);
    } 
    //parent process
    close(fds[1]); //close write
    char inbuf[256]; //fixed raw buffer to recieve chunks of data from pipe 256 bytes at a time
    std::string output;
    while ((nbytes = read(fds[0], inbuf, sizeof(inbuf))) > 0) //read loop, read function returns num of bytes written into inbuf
        output.append(inbuf, nbytes); //append the chars specified by nbytes

    std::stringstream ss(output);
    std::string word;
    while (ss >> word) {
        res.push_back(word);
    }
    close(fds[0]); //close read
    waitpid(pid, &status, 0); 
    return res;

}

std::string find_path(const std::string& arg) {
    const char* p = std::getenv("PATH");
    if (!p) return "";

    std::string path = p;
    std::stringstream ss(path);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) continue;
        fs::path full_path = dir + "/" + arg;
        if (access(full_path.c_str(), X_OK) == 0) {
            return full_path.string();
        }
    }
    return "";
}

int execute(const std::string& exe_path, const std::string& command,
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

std::string read_input() {
    std::string loc_buffer;
    char c;
    int tab_count = 0;
    while (read(STDIN_FILENO, &c, 1) > 0) { //while input reading doesnt return 0 bytes
        if (c == '\n') { std::cout << '\n'; break; } //enter
        else if (c == '\t')
        { 
            std::string s = "";
            if (loc_buffer.find(' ') != std::string::npos) {
                size_t pos = loc_buffer.rfind(' ');
                std::string arg = loc_buffer.substr(pos + 1);
                std::string command_name = loc_buffer.substr(0, loc_buffer.find(' '));
                auto it = complete_paths.find(command_name);
                if (it != complete_paths.end()) {
                    //handle case where the is custom complete specification for a cmd
                    tab_count++;
                    s = run_completer(complete_paths[command_name].path(), command_name, arg, loc_buffer, tab_count); //run completer
                } else if (arg.rfind('/') != std::string::npos) {
                    tab_count++;
                    size_t slash_pos = arg.rfind('/'); //position of the slash
                    s = path_completion(arg, loc_buffer, slash_pos, tab_count); // this method returns the string of the completed path
                    //std::cout << " full path " << s << std::endl;
                } else {
                    tab_count++;
                    s = completion(filename_trie, arg, loc_buffer, tab_count);
                }
                s = loc_buffer.substr(0, pos) + " " + s;
                //std::cout << "value of s " << s << std::endl;
                
            } else {
                tab_count++; 
                s = completion(builtin_trie, loc_buffer, loc_buffer, tab_count); //on tab press check trie
            }
            if (s != loc_buffer) {
                loc_buffer = s;
                tab_count = 0;
            }
        }
        else if (c == 127) {
            tab_count = 0;
            if (!loc_buffer.empty()) {
                loc_buffer.pop_back();
                std::cout << "\b \b" << std::flush; //backspace
            }
        } else {
            tab_count = 0;
            loc_buffer += c;
            std::cout << c;

            {
                std::lock_guard<std::mutex> lock(i_mutex);
                current_input = loc_buffer;
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(i_mutex);
        current_input.clear();
    }
    return loc_buffer;
}

std::string completion(Trie& trie, std::string cur_input, const std::string& full_line, int tab_count) {
    //if (cur_input.empty()) return cur_input;

    std::vector<std::string> matches = trie.get_children(cur_input);
    return matches_helper(matches, cur_input, full_line, tab_count);
}

void parse(const std::string& command, std::vector<std::string>& tokens) {
    std::string cur = "";
        bool iq = false;   // inside single quotes
        bool idq = false;  // inside double quotes
        for (size_t i = 0; i < command.size(); i++) {
            char c = command[i];
            if (c == '\\' && !iq && !idq) {         // backslash outside quotes
                if (i + 1 < command.size()) {
                    cur += command[++i];
                }
            } else if (c == '\\' && idq) {          // backslash inside double quotes
                if (i + 1 < command.size()) {
                    char next = command[i + 1];
                    if (next == '"' || next == '\\') {
                        cur += command[++i];
                    } else {
                        cur += c;
                    }
                }
            } else if (c == '\"' && !idq && !iq) {
                idq = true;
            } else if (c == '\'' && !iq && !idq) {
                iq = true;
            } else if (c == '\'' && iq) {
                iq = false;
            } else if (c == '\"' && idq) {
                idq = false;
            } else if (c == ' ' && !iq && !idq) {
                if (!cur.empty()) {
                    tokens.push_back(cur);
                    cur = "";
                }
            } else {
                cur += c;
            }
        }

        if (!cur.empty()) tokens.push_back(cur);
}

void populate_from_path() {
    const char* p = std::getenv("PATH"); //gets val of path env variable
    if (!p) return; //if nullptr return

    std::stringstream ss(p); //read like a stream
    std::string dir;
    while (std::getline(ss, dir, ':')) { //insert into dir
        if (dir.empty()) continue;
        if (!fs::exists(dir)) continue; //check if exists
        try {
            for (const auto &entry : fs::directory_iterator(dir)) { //loops through file and folders in the dir
                try {
                    if (fs::is_regular_file(entry) && access(entry.path().c_str(), X_OK) == 0) { //if file is executable
                        builtin_trie.insert(entry.path().filename().string());
                    }
                } catch(...) {
                    continue;
                }
        }
        } catch (...) {
            continue;
        }
    }
}

std::string longest_common_prefix(const std::vector<std::string>& matches) {
    if (matches.empty()) return "";
    if (matches.size() == 1) return matches[0];
    std::string lcp = "";
    for (size_t i = 0; i < matches[0].size(); i++) {
        char c = matches[0][i];

        for (size_t j = 1; j < matches.size(); j++) {
            if (i >= matches[j].size() || matches[j][i] != c) {
                return lcp;
            }
        }
        lcp += c;
    }
    return lcp;
}

void populate_files() {
    for (const auto &entry : fs::directory_iterator(fs::current_path())) {
        if (fs::is_regular_file(entry) || fs::is_directory(entry)) {
            filename_trie.insert(entry.path().filename().string());
        }
    }
}

std::string path_completion(const std::string& s, const std::string& full_line, size_t s_pos, int tab_count)  
{
    std::string dir_path = s.substr(0, s_pos + 1); //returns the directory path before the last /
    std::string prefix = s.substr(s_pos + 1); //whatever is after the slash
    std::vector<std::string> matches;
    try {
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            std::string name = entry.path().filename().string(); //every file in that directory
            if (name.rfind(prefix, 0) == 0) matches.push_back(name); //if the prefix exists in name add to matches
        }

    } catch(...) {}
    
    return matches_helper(matches, prefix, full_line, tab_count, dir_path);
}

std::string matches_helper(std::vector<std::string>& matches, const std::string& cur_input,
                             const std::string& full_input, int tab_count, const std::string& dir_path) {

    if (matches.empty()) { 
        std::cout << "\x07" << std::flush;
        return dir_path + cur_input;
    }
    std::string lcp = longest_common_prefix(matches);
    if (matches.size() == 1) {
        std::string suffix = lcp.substr(cur_input.size());
        std::string full = dir_path + lcp;
        char trailing_char = fs::is_directory(full) ? '/' : ' ';
        std::cout << suffix << trailing_char << std::flush;
        return full + trailing_char;
    }

    if (lcp.size() > cur_input.size()) {
        std::string suffix = lcp.substr(cur_input.size());
        std::cout << suffix << std::flush;
        return dir_path + lcp;
    }

    if (tab_count == 1) {
        std::cout << "\x07" << std::flush;
        return dir_path + cur_input;
    }

    std::sort(matches.begin(), matches.end());
    std::cout << "\n";
    for (size_t i = 0; i < matches.size(); i++) {
        if (i > 0) std::cout << "  ";
        std::string full = dir_path + matches[i];
        std::cout << matches[i] << (fs::is_directory(full) ? "/" : "");
    }
    std::cout << "\n$ " << full_input << std::flush;
    return cur_input;
}
