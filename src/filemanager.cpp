#include "./headers/filemanager.h"
#include <unistd.h>
#include <filesystem>
#include <fcntl.h>
#include "./headers/trie.h"

int FileManager::redirect_fd(int fd_num, int FLAG_CONST, const std::string& path, Trie* filename_trie) {
    if (path.empty()) return -1;
    bool file_existed = std::filesystem::exists(path); //checks if file already exists
    int saved = dup(fd_num);
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | FLAG_CONST, 0644);
    if (fd < 0) {
        perror("open");
        close(saved);
        return -1;
    }
    if (!file_existed) { //if it doesnt exist we need to add it to the trie during runtime
        std::filesystem::path file_path(path);
        filename_trie->insert(file_path.filename().string());
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