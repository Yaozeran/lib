#include "timewheel.hpp"


#include <cstddef>


int main() {

    FixedThreadPool threadpool = FixedThreadPool(4);

    std::shared_ptr<FixedThreadPool> ptr = std::make_shared<FixedThreadPool>(threadpool);

    auto secTimeWheel = TimeWheel<int, std::chrono::seconds>(60, 1, ptr);

    return 0;
}
