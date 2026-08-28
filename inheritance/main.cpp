#include <iostream>

class Entity {
    float x, y;
    int speed;

public:
    void Move(int xa, int ya) {
        x += xa * speed;
        y += ya * speed;
    }
};

// private by default
// prefer if inheritance is needed
class Player : public Entity {
    std::string name;

public:
    void Print() {
        std::cout << name << std::endl;
    }
    void SetName(std::string username) {
        name = username;
    }
};

int main() {
    Player player;
    std::cout << sizeof(Player) << std::endl;

    player.SetName("AI");
    player.Print();
    player.Move(3, 1);

    std::cin.get();
}