#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>

int main() {
    std::cout << "vector\n";
    std::vector<int> v{1, 2, 3};
    v.push_back(4);
    v.emplace_back(5);
    std::cout << "  size=" << v.size() << " capacity=" << v.capacity() << "\n";
    std::cout << "  values: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    // growth
    std::vector<int> growing;
    for (int i = 0; i < 10; ++i) {
        growing.push_back(i);
        std::cout << "  after push " << i << "  size=" << growing.size()
                  << " capacity=" << growing.capacity() << "\n";
    }

    std::cout << "map (ordered by key)\n";
    std::map<std::string, int> ages;
    ages["alice"] = 30;
    ages["bob"] = 25;
    ages.insert({"carol", 28});
    for (const auto& [name, age] : ages)          // structured binding
        std::cout << "  " << name << " → " << age << "\n";

    std::cout << "unordered_map (hash)\n";
    std::unordered_map<std::string, int> scores{{"alice", 95}, {"bob", 87}};
    scores["carol"] = 92;
    for (const auto& [name, score] : scores)
        std::cout << "  " << name << " → " << score << "\n";

}