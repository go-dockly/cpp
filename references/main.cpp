#include <iostream>

#define LOG(x) std::cout << x << std::endl;

void increment_ptr(int* value) {
    (*value)++;
}

void increment_reference(int& value) {
    value++;
}

int main() {
    int a = 1;
    int& ref_a = a; // alias
    ref_a = 5;
    LOG(ref_a);

    a = 10;
    int b = 6;

    int* ref = &a;
    *ref = 2; // ref to a
    ref = &b;
    *ref = 1; // ref to b
    
    LOG("a");
    LOG(a);
    LOG("b");
    LOG(b);

    increment_reference(a);
    LOG(a);

    increment_ptr(&a);
    LOG(a);
    
    std::cin.get();
}