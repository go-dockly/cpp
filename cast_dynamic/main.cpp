#include <iostream>

#define LOG(x) std::cout << x << std::endl;

class Entity {
public:
    virtual ~Entity() = default; // enable dynamic_cast and prevent mem leaks
};

class Player : public Entity {
};

class Enemy : public Entity {
};

int main() {
    Player* player = new Player(); // heap alloc
    Entity* p0 = player;

    Entity* enemy = new Enemy();

    Player* p1 = dynamic_cast<Player*>(enemy);

    if (p1 == nullptr) {
        LOG("Cast failed: enemy is not a Player!");
    }
    std::cin.get();
}