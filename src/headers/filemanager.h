#pragma once

#include <string>

class FileManager {
public:
    static int redirect_fd(int fd_num, int FLAG_CONST, const std::string& path);
    static void restore_fd(int fd_num, int saved_fd);
};
