#pragma once

#include <map>
#include <thread>
#include <mutex>
#include "job.h"

class JobsBuiltin {
private: 
    std::map<int, Job> jobs;
    std::mutex j_mutex;
public:
    void handle_builtin();
    std::map<int, Job> get_jobs();
    void add_job(pid_t pid, std::string full_cmd);
};