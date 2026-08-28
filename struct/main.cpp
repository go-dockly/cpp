#include <iostream>

// public by default
// prefer structs if compat with C is needed 
// or plain old data eg math.vec2 
struct PlayerSruct {
    int x, y;
    int speed;

    void Move(int xa, int ya) {
        x += xa * speed;
        y += ya * speed;
    }
    void Position() {
        std::cout << "x:" << x << " y:" << y << std::endl;
    }
};

// private by default
// prefer if inheritance is needed
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
    player1.Position();

    PlayerClass player2;
    player2.Move(3, 1);

    std::cin.get();
}