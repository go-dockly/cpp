#include <iostream>

#define LOG(x) std::cout << x << std::endl;

void* operator new(size_t size) {
    std::cout << "Allocating size " << size << std::endl;
    return malloc(size);
}

void operator delete(void* memory, size_t size) noexcept {
    std::cout << "Freeing size " << size << std::endl;
    free(memory);
}

void operator delete(void* memory) noexcept {
    std::cout << "Free memory (unsized)" << std::endl;
    free(memory);
}

struct Object {
    int x, y, z;
};

int main() {
    {
        std::unique_ptr<Object> obj = std::make_unique<Object>();
    }
    std::cin.get();
}