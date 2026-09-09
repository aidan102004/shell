#pragma once

#include <string>

struct Job {
    int j_num;
    pid_t process_id;
    std::string command_str;
    bool status;
};