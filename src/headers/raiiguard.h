#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include "filemanager.h"
#include "trie.h"

//RAII class to handle redirects automatically
class RAIIGuard {
private:
    int saved_stdout;
    int saved_stderr;
public:
    RAIIGuard(const std::string& out_file, const std::string& err_file, int flags, Trie* filename_trie) :
    saved_stdout(-1), saved_stderr(-1) {
        //redirect only if files are specified
        if (!out_file.empty()) 
            saved_stdout = FileManager::redirect_fd(STDOUT_FILENO, flags, out_file, filename_trie);
        if (!err_file.empty()) 
            saved_stderr = FileManager::redirect_fd(STDERR_FILENO, flags, err_file, filename_trie);
    }

    //automatically restores when scope ends
    ~RAIIGuard() {
        if (saved_stdout != -1) 
            FileManager::restore_fd(STDOUT_FILENO, saved_stdout);
        if (saved_stderr != -1) 
            FileManager::restore_fd(STDERR_FILENO, saved_stderr);
    }

    //prevent copying so we dont even have two objects managing the same file descripter 
    RAIIGuard(const RAIIGuard&) = delete; //prevents creating new object passing in an existing one
    RAIIGuard& operator=(const RAIIGuard&) = delete; //prevents assigning exisitng object to new one
};
