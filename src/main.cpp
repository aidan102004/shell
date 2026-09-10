#include <iostream>
#include <string>
#include <mutex>
#include <filesystem>
#include <unistd.h>
#include <termios.h>
#include "./headers/shell.h"

namespace fs = std::__fs::filesystem;

std::string current_input;
std::mutex i_mutex;

//shell object
Shell shell;

struct termios original_termios;

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
    shell.setup_trie(); //populate trie
    while (true) {
        std::cout << "$ ";
        enable_raw(); //swap from canonical to raw
        std::string command = shell.read_input(); //read input
        if (command.empty()) continue;
        shell.dispatch(command); //start dispatch process
    }
}
