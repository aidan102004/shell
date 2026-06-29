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

namespace fs = std::__fs::filesystem;

// Forward declarations
void handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins);
std::string find_path(const std::string& arg);
void execute(const std::string& exe_path, const std::string& command, const std::vector<std::string>& tokens, const std::string& redirect_file, const std::string& redirect_stderr, int FLAG_CONST);
void handle_cd(const std::string& arg);

// Builtin commands list
std::unordered_set<std::string> commands = {
    "echo", "exit", "type", "pwd", "cd"
};

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

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    while (true) {
        std::cout << "$ ";
        std::string command;
        std::getline(std::cin, command);

        std::vector<std::string> tokens;

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
        if (tokens.empty()) continue;

        std::string redirect_file = "";
        int FLAG_CONST = O_TRUNC;
        std::string redirect_stderr = "";
        std::vector<std::string> clean_tokens;

        for (size_t i = 0; i < tokens.size(); i++) {
            if ((tokens[i] == ">" || tokens[i] == "1>") && i + 1 < tokens.size()) {
                redirect_file = tokens[i + 1];
                i++; // skip filename token too
            } else if ((tokens[i] == ">>" || tokens[i] == "1>>") && i + 1 < tokens.size()) {
                redirect_file = tokens[i + 1];
                FLAG_CONST = O_APPEND;
                i++; 
            }
            else if (tokens[i] == "2>" && i + 1 <tokens.size()) {
                redirect_stderr = tokens[i + 1];
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