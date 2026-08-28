#include <iostream>

#define LOG(x) std::cout << x << std::endl;

struct Entity {
    int x,y;
};

int main() {
    Entity e = { 1,2 };
    int* position = (int*)&e;

    int x = *(int*)((char*)&e);
    LOG(x);

    int y = *(int*)((char*)&e + 4); // the raw power of C++
    LOG(y);

    std::cin.get();
}