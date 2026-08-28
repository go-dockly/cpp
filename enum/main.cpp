#include <iostream>

#define LOG(x) std::cout << x << std::endl;

enum class Example : int {   // optional type
    A = 5,
    B,                       // 6
    C                        // 7
};

int main() {
    Example value = Example::B;

    // enum does not implicitly convert to int
    LOG(static_cast<int>(value));

    // compare directly
    if (value == Example::B) {
        LOG("value is B");
    }

    std::cin.get();
}