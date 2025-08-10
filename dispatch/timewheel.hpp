/**
 * 
 */


#pragma once


#include <cstddef>
#include <chrono>

#include "../concurrent/threadpool.hpp"


template<typename D, size_t len>
class TimeWheel {

private:

    size_t len;

    std::chrono::seconds dur;

public:
    
    TimeWheel(size_t len, std::chrono::seconds dur) {}

};