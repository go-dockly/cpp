#include <iostream>

class Entity {
public:
    // constructor
    Entity() {
        std::cout << "Created Entity" << std::endl;
    }
    // destructor
    ~Entity() {
        std::cout << "Destroyed Entity" << std::endl;
    }

    void Print() {

    }
};

int main() {
    // heap alloc
    // std::unique_ptr<Entity> entity(new Entity());
    std::unique_ptr<Entity> entity = std::make_unique<Entity>();

    std::shared_ptr<Entity> sharedEntity = std::make_shared<Entity>();
    std::shared_ptr<Entity> e0 = sharedEntity;
    {
        std::weak_ptr<Entity> weak = e0; // ask if expired
    };

    // unique_ptr always first preference / shared_ptr second

    entity->Print();

}