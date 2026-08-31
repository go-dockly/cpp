#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

int main() {
    auto greet = []() { std::cout << "Hello from lambda\n"; };
    greet();

    // Capture by value [=] & reference [&]
    int x = 10;
    auto by_value = [=]() { std::cout << "x by value: " << x << "\n"; };
    auto by_ref   = [&]() { x += 5; std::cout << "x by ref: " << x << "\n"; };
    by_value();
    by_ref();

    // C++14

    // Explicit captures + init-capture
    int y = 20;
    auto init_cap = [z = y + 1]() { std::cout << "init-capture z: " << z << "\n"; };
    init_cap();

    // Generic lambda
    auto add = [](auto a, auto b) { return a + b; };
    std::cout << "generic: " << add(3, 4.5) << "\n";

    // Mutable lambda modifies captured-by-value
    auto counter = [n = 0]() mutable { return ++n; };
    std::cout << "counter: " << counter() << " " << counter() << "\n";

    // Lambda as algorithm predicate
    std::vector<int> v{5, 1, 8, 3, 9};
    std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; }); // descending
    for (int n : v) std::cout << n << " ";
    std::cout << "\n";

    // C++23

    // Recursive lambda
    std::function<int(int)> fib = [&](int n) {
        return n <= 1 ? n : fib(n-1) + fib(n-2);
    };
    std::cout << "fib(6): " << fib(6) << "\n";
}
