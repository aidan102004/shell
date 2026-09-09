#include "jobsbuiltin.h"
#include "global.h"
#include <iostream>
#include <iomanip>

void JobsBuiltin::handle_builtin() {
    const int pad_const = 24; 
    std::lock_guard<std::mutex> lock(j_mutex);

    for (const auto& [order, job] : jobs) 
    {
        int pad_delta = pad_const - std::to_string(abs(static_cast<int>(job.process_id))).size(); //27 characters of padding
        std::string status = (job.status == true) ? "Running" : "Done"; //so far this will always be true
        char marker = ' ';
        if (jobs.size() == 1 || job.j_num == jobs.rbegin()->first) {
            marker = '+';
        } else if (job.j_num == std::next(jobs.rbegin())->first) {
            marker = '-';
        }
        std::cout << "[" << job.j_num << "]" << marker << "  " << status << std::setw(pad_delta) << job.command_str << std::endl;;
    }
}

std::map<int, Job> JobsBuiltin::get_jobs() {
    return jobs;
}

void JobsBuiltin::add_job(pid_t pid, std::string full_cmd) 
{
    //lambda func
    auto monitor_func = [this](Job& job) {
        int status;
        pid_t res = waitpid(job.process_id, &status, 0); //waits for process to finish
        if (res == job.process_id) {
            std::string snapshot;
            {
                std::lock_guard<std::mutex> lock(i_mutex);
                snapshot = current_input; //get current input
            }
            int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
            std::cout << "\r\033[K"; //clear terminal
            if (exit_code == 0) std::cout << "[" << job.j_num << "]+  Done    " << job.command_str << std::endl;
            else std::cout << "[" << job.j_num << "]+  Exit " << exit_code << "  " << job.command_str << std::endl;
            std::cout << "$ " << snapshot << std::flush; //reprint input snapshot
            job.status = false;
            {
                std::lock_guard<std::mutex> lock(j_mutex);
                jobs.erase(job.j_num); //remove job
            }
        }
    };
    //create job
    Job* job_ptr; //store a pointer
    int next_id;
    {
        std::lock_guard<std::mutex> lock(j_mutex);
        next_id = !jobs.empty() ? jobs.rbegin()->first + 1 : 1; //increment ID to be 1 greater than current largest which will always be latest added
        jobs[next_id] = {next_id, pid, full_cmd, true}; //add job to hashmap
        job_ptr = &jobs[next_id]; //assign ptr
    }
    std::cout << "[" << next_id << "] " << pid << std::endl;
    std::thread t(monitor_func, std::ref(*job_ptr)); //create thread passing func ptr and ref of dereferenced job_ptr which stores our job
    t.detach(); //run this concurrently dont wait
}