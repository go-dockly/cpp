#include <iostream>
#include <vector>

int main() {

    std::vector<int> values = {1, 2, 3, 4, 5, 6};

    for (std::size_t i = 0; i < values.size(); i++) {
        std::cout << values[i] << std::endl;
    };

    for (int value : values) {
        std::cout << value << std::endl;
    }
    
    for (std::vector<int>::iterator it = values.begin(); it != values.end(); it++) {
        std::cout << *it << std::endl;
    }

    using ScoreMap = std::unordered_map<std::string, int>;
    ScoreMap map;

    map["C++"] = 1;
    map["Me"] = 0;

    // custom 0ld school
    for (ScoreMap::const_iterator it = map.begin(); it != map.end(); it++) {
        auto& key = it->first;
        auto& value = it->second;

        std::cout << key << ":" << value << std::endl;
    }

    // modern & clean
    for (auto [key, value] : map)
        std::cout << key << ":" << value << std::endl;

    std::cin.get();
}