#include "./builtins/completebuiltin.h"
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include "./headers/util.h"

void CompleteBuiltin::handle_complete_builtin(const std::vector<std::string>& args) {
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
std::string CompleteBuiltin::run_completer(const fs::path& script, const std::string& command, const std::string& curr_word, const std::string& full_input, int tab_count) {
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
    return Util::matches_helper(candidates, curr_word, full_input, tab_count);
}
std::vector<std::string> CompleteBuiltin::execute_completer(const fs::path& script, const std::string& command, const std::string& curr_word, const std::string& prev_word, const std::string& f_in) {
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