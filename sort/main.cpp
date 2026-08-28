#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

#define LOG(x) std::cout << x << std::endl;

int main() {
    std::vector<int> values = { 3,5,1,4,2 };
    
    std::sort(values.begin(), values.end(), [](int a, int b) {
        if (a==1)
            return false;
        if (b==1)
            return true;
        return a>b;
    });

    for (int value : values)
        LOG(value)

    std::cin.get();
}