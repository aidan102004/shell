#include <iostream>
#include <string>
#include <unordered_set>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include "trie.h"

namespace fs = std::__fs::filesystem;

// Forward declarations
void handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins);
std::string find_path(const std::string& arg);
void execute(const std::string& exe_path, const std::string& command, const std::vector<std::string>& tokens, const std::string& redirect_file, const std::string& redirect_stderr, int FLAG_CONST);
void handle_cd(const std::string& arg);
std::string read_input();
std::string completion(Trie& trie, std::string cur_input, int tab_count);
void parse(const std::string& command, std::vector<std::string>& tokens);
void populate_from_path();
std::string longest_common_prefix(const std::vector<std::string>& matches);
void populate_files();
std::string path_completion(const std::string& s, size_t s_pos);

// Builtin commands list
std::unordered_set<std::string> commands = {
    "echo", "exit", "type", "pwd", "cd"
};

Trie builtin_trie;
Trie filename_trie;
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
        char c;
        enable_raw(); //swap from canonical to raw
        std::string command = read_input(); //read input
        std::vector<std::string> tokens;
        parse(command, tokens); //parse input
        if (tokens.empty()) continue;

        std::string redirect_file = ""; 
        int FLAG_CONST = O_TRUNC; //default for file redirection
        std::string redirect_stderr = "";
        std::vector<std::string> clean_tokens;

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
            } 
            else {
                clean_tokens.push_back(tokens[i]);
            }
        }

        if (clean_tokens.empty()) continue;
        std::string cmd = clean_tokens[0];

        if (cmd == "exit") {
            break;
        } else if (cmd == "type") {
            int saved_out = redirect_fd(STDOUT_FILENO, FLAG_CONST, redirect_file);
            int saved_err = redirect_fd(STDERR_FILENO, FLAG_CONST, redirect_stderr);
            handle_type(clean_tokens.size() > 1 ? clean_tokens[1] : "", commands);
            restore_fd(STDOUT_FILENO, saved_out);
            restore_fd(STDERR_FILENO, saved_err);
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
        } else if (cmd == "pwd") {
            int saved_out = redirect_fd(STDOUT_FILENO, FLAG_CONST, redirect_file);
            int saved_err = redirect_fd(STDERR_FILENO, FLAG_CONST, redirect_stderr);
            std::cout << fs::current_path().string() << std::endl;
            restore_fd(STDOUT_FILENO, saved_out);
            restore_fd(STDERR_FILENO, saved_err);
        } else if (cmd == "cd") {
            handle_cd(clean_tokens.size() > 1 ? clean_tokens[1] : "");
        } else {
            execute(find_path(cmd), command, clean_tokens, redirect_file, redirect_stderr, FLAG_CONST);
        }
    }

    return 0;
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

void execute(const std::string& exe_path, const std::string& command,
             const std::vector<std::string>& tokens, const std::string& redirect_file, const std::string& redirect_stderr, int FLAG_CONST) {
    if (exe_path.empty()) {
        std::cerr << command << ": command not found" << std::endl;
        return;
    }

    std::vector<char*> argv;
    for (auto& t : tokens) {
        argv.push_back(const_cast<char*>(t.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
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
}

std::string read_input() {
    std::string input;
    char c;
    int tab_count = 0;
    while (read(STDIN_FILENO, &c, 1) > 0) { //while input reading doesnt return 0 bytes
        if (c == '\n') { std::cout << '\n'; break; } //enter
        else if (c == '\t')
        { 
            std::string s = "";
            if (input.find(' ') != std::string::npos) {
                size_t pos = input.find(' ');
                std::string arg = input.substr(pos + 1);
                if (arg.rfind('/') != std::string::npos) {
                    size_t slash_pos = arg.rfind('/'); //position of the slash
                    s = path_completion(arg, slash_pos); // this method returns the string of the completed path
                } else {
                    s = completion(filename_trie, arg, tab_count);
                }
                s = input.substr(0, pos) + " " + s;
                
            } else {
                tab_count++; 
                s = completion(builtin_trie, input, tab_count); //on tab press check trie
            }
            if (s != input) {
                input = s;
                tab_count = 0;
            }
        }
        else if (c == 127) {
            tab_count = 0;
            if (!input.empty()) {
                input.pop_back();
                std::cout << "\b \b" << std::flush; //backspace
            }
        } else {
            tab_count = 0;
            input += c;
            std::cout << c;
        }
    }
    return input;
}

std::string completion(Trie& trie, std::string cur_input, int tab_count) {
    //if (cur_input.empty()) return cur_input;

    std::vector<std::string> matches = trie.get_children(cur_input);
    if (matches.empty()) {
        std::cout << "\x07" << std::flush;
        return cur_input;
    }

    std::string lcp = longest_common_prefix(matches);

    if (matches.size() == 1) {
        std::string suffix = lcp.substr(cur_input.size());
        char trailing_char = fs::is_directory(lcp) ? '/' : ' ';
        std::cout << suffix << trailing_char << std::flush;
        return lcp + trailing_char;
    }

    if (lcp.size() > cur_input.size()) {
        std::string suffix = lcp.substr(cur_input.size());
        std::cout << suffix << std::flush;
        return lcp;
    }

    if (tab_count == 1) {
        std::cout << "\x07" << std::flush;
        return cur_input;
    }

    std::sort(matches.begin(), matches.end());
    std::cout << "\n";
    for (size_t i = 0; i < matches.size(); i++) {
        if (i > 0) std::cout << "  ";
        std::cout << matches[i];
    }
    std::cout << "\n$ " << cur_input << std::flush;
    return cur_input;
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

std::string path_completion(const std::string& s, size_t s_pos)  
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
    //if we have one match
    if (matches.size() == 1) {
        std::string full_path = dir_path + matches[0]; 
        std::string suffix = matches[0].substr(prefix.size());
        char trailing_char = fs::is_directory(full_path) ? '/' : ' ';
        std::cout << suffix << trailing_char << std::flush;
        return full_path + trailing_char;
    }

    std::cout << "\x07" << std::flush;
    return s;
}