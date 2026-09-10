#include "./headers/filemanager.h"
#include <unistd.h>
#include <filesystem>
#include <fcntl.h>

int FileManager::redirect_fd(int fd_num, int FLAG_CONST, const std::string& path) {
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
void FileManager::restore_fd(int fd_num, int saved_fd) {
    if (saved_fd == -1) return;
    dup2(saved_fd, fd_num);
    close(saved_fd);
}