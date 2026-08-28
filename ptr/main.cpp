#include <iostream>

#define LOG(x) std::cout << x << std::endl;


int main() {
    int a = 8; // stack assign
    int* ptr = &a; // point to ref of a
    *ptr = 10; // deref ptr & assign value
    LOG(a);

    char* buffer = new char[8]; // alloc 8 bytes of mem and return ptr to mem block
    memset(buffer, 0, 8);

    char** ptr = &buffer; // ptr to ptr (double ptr) set to mem address of buffer 

    delete[] buffer;

    std::cin.get();
}