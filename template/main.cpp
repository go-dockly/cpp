#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// func template
template <typename T, typename U>
constexpr auto max_value(T a, U b) {
    using Common = std::common_type_t<T, U>;
    return (static_cast<Common>(a) > static_cast<Common>(b))
               ? static_cast<Common>(a)
               : static_cast<Common>(b);
}

// class template
template <typename T>
class Box {
    T value;
public:
    explicit Box(T v) : value(std::move(v)) {}
    const T& get() const { return value; }
    void set(T v) { value = std::move(v); }
};

// specialization for ptrs
template <typename T>
class Box<T*> {
    T* ptr;
public:
    explicit Box(T* p) : ptr(p) {}
    T* get() const { return ptr; }
};

// non-type template params
template <typename T, size_t N>
class FixedArray {
    T data[N];
public:
    T& operator[](size_t i) { return data[i]; }
    size_t size() const { return N; }
};

int main() {
    std::cout << "func template\n";
    std::cout << "  max(3, 7.21) = " << max_value(3, 7.21) << "\n";

    std::cout << "class template\n";
    Box<int> bi(42);
    Box<std::string> bs("hello");
    std::cout << "  Box<int> = " << bi.get() << "\n";
    std::cout << "  Box<string> = " << bs.get() << "\n";

    std::cout << "ptr template\n";
    int x = 99;
    Box<int*> bp(&x);
    std::cout << "  Box<int*> points to " << *bp.get() << "\n";

    std::cout << "non-type param\n";
    FixedArray<int, 3> arr;

    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    std::cout << "  FixedArray size = " << arr.size() << " values: "
              << arr[0] << " " << arr[1] << " " << arr[2] << "\n";
}