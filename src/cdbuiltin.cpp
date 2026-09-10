#include "./builtins/cdbuiltin.h"
#include <iostream>
#include <unistd.h>

void CdBuiltin::handle_cd(const std::string& arg) {
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