#pragma once

#include <chrono>
#include <iostream>
#include <type_traits>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION_STREAM(x, y) LogDuration UNIQUE_VAR_NAME_PROFILE(x, y)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

class LogDuration {
public:

    using Clock = std::chrono::steady_clock;

    

    LogDuration(const std::string& id, std::ostream& flow = std::cerr )
        : id_(id), flow_(flow) {
    }

    void Print(){
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;

        //flow_ << "\033[6;32mbold red text\033[0m\n" << std::endl;
        flow_  << "\033[1;32m             ┌————————————————————————\n";
        flow_ <<  "LOG >>       │" << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms  \n"s;
        flow_  << "             └————————————————————————\033[0m\n";
        printed = true;
    }

    ~LogDuration() {
        if(!printed){
            Print();
        }
    }

    

private:
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& flow_;
    bool printed = false;
};