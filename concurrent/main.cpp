#include "threadpool.hpp"


#include <iostream>
#include <functional>


int mult(int a, int b) {
    return a * b;
}


int main() {
    {
    FixedThreadPool executor = FixedThreadPool(4);

    std::function<int(int, int)> sum = [] (int a, int b) -> int {
        return a + b;
    };

    auto res = executor.Exec(sum, 1 , 2);
    std::cout << res.get() << std::endl;
    }

    return 0;
};