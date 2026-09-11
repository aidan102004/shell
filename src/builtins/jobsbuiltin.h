#pragma once

#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include "../headers/job.h"

class JobsBuiltin {
private: 
    std::map<int, Job> jobs;
    std::mutex j_mutex;
public:
    void handle_builtin();
    std::map<int, Job> get_jobs();
    void add_job(pid_t pid, std::string full_cmd);
    void handle_bg(const std::vector<std::string>& tokens);
    void handle_fg(const std::vector<std::string>& tokens);
};