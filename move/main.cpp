#include <iostream>
#include <utility>
#include <string>
#include <vector>

struct Resource {
    std::string name;
    explicit Resource(std::string n) : name(std::move(n)) {
        std::cout << "  construct " << name << "\n";
    }
    Resource(const Resource& other) : name(other.name) {
        std::cout << "  COPY " << name << "\n";
    }
    Resource(Resource&& other) noexcept : name(std::move(other.name)) {
        std::cout << "  MOVE " << name << "\n";
    }
    Resource& operator=(Resource&& other) noexcept {
        name = std::move(other.name);
        std::cout << "  MOVE-ASSIGN " << name << "\n";
        return *this;
    }
    ~Resource() { if (!name.empty()) std::cout << "  destroy " << name << "\n"; }
};

// forward factory
template <typename T, typename... Args>
T make(Args&&... args) {
    return T(std::forward<Args>(args)...);  // preserves lvalue/rvalue
}

void take_by_value(Resource r) {
    std::cout << "  took ownership of " << r.name << "\n";
}

int main() {
    std::cout << "move construct\n";
    Resource a{"A"};
    Resource b = std::move(a);          // move ctor
    // name is now empty

    std::cout << "move into container\n";
    std::vector<Resource> v;
    v.push_back(Resource{"temp"});      // move from temp
    v.emplace_back("direct");           // construct in place

    std::cout << "perfect forwarding\n";
    auto r1 = make<Resource>("from string");          // rvalue → move
    std::string s = "from lvalue";
    auto r2 = make<Resource>(s);                      // lvalue → copy
    auto r3 = make<Resource>(std::move(s));           // force move

    std::cout << "passing to func\n";
    take_by_value(std::move(r1));                     // move into param
}
