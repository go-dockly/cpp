#include <iostream>

#define LOG(x) std::cout << x << std::endl;

// public by default
// prefer structs if compat with C is needed 
// plain old data eg math.vec2 
struct PlayerSruct {
    int x, y;
    int speed;

    void Move(int xa, int ya) {
        x += xa * speed;
        y += ya * speed;
    }
};

// private by default
class PlayerClass {
    int x, y;
    int speed;

public:
    void Move(int xa, int ya) {
        x += xa * speed;
        y += ya * speed;
    }
};

int main() {
    PlayerSruct player1;
    player1.Move(3, 1);

    PlayerClass player2;
    player2.Move(3, 1);

    std::cin.get();
}