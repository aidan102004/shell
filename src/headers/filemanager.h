#pragma once

#include <string>
#include "trie.h"

class FileManager {
public:
    static int redirect_fd(int fd_num, int FLAG_CONST, const std::string& path, Trie* filename_trie);
    static void restore_fd(int fd_num, int saved_fd);
};
