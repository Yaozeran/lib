#include "threadpool.hpp"


#include <functional>


int main() {
    ThreadPool<8, 4> executor;

    std::function<int(int, int)> sum = [] (int a, int b) -> int {
        return a + b;
    };

    auto res = executor.Exec(sum, 1 , 2);

    return 0;
};