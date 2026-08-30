#include <iostream>
#include <map>
#include <tuple>
#include <string>
#include <utility>

struct Point {
    int x;
    int y;
};

std::pair<std::string, int> get_person() {
    return {"Alice", 30};
}

std::tuple<int, double, std::string> get_data() {
    return {42, 3.14, "pi"};
}

int main() {
    // pair / tuple
    auto [name, age] = get_person();
    std::cout << name << " is " << age << "\n";

    auto [i, d, s] = get_data();
    std::cout << i << " " << d << " " << s << "\n";

    // struct / array
    Point p{10, 20};
    auto [x, y] = p;
    std::cout << "Point: " << x << "," << y << "\n";

    int arr[] = {1, 2, 3};
    auto [a, b, c] = arr;
    std::cout << a << " " << b << " " << c << "\n";

    // map iteration
    std::map<std::string, int> scores{{"Bob", 90}, {"Carol", 85}};
    for (const auto& [key, value] : scores) {
        std::cout << key << " → " << value << "\n";
    }

    // modify with references
    auto& [rx, ry] = p;
    rx = 100;
    std::cout << "modified Point: " << p.x << "," << p.y << "\n";

    // ignore values with [[maybe_unused]] or _
    auto [id, _, label] = get_data();
    std::cout << "id=" << id << " label=" << label << "\n";
}
