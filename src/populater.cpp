#include "./headers/populater.h"
#include <unistd.h>
#include <sstream>
#include <string>
#include <fstream>
#include "./headers/trie.h"

namespace fs = std::__fs::filesystem;

void Populater::populate_from_path(Trie* builtin_trie) {
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
                        builtin_trie->insert(entry.path().filename().string());
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

void Populater::populate_files(Trie* filename_trie) {
    for (const auto &entry : fs::directory_iterator(fs::current_path())) {
        if (fs::is_regular_file(entry) || fs::is_directory(entry)) {
            filename_trie->insert(entry.path().filename().string());
        }
    }
}

