#include <iostream>
#include <thread>
#include <chrono>

static bool finished = false;

void work() {
    using namespace std::literals::chrono_literals;

    std::cout << "Started thread id " << std::this_thread::get_id() << std::endl;

    while (!finished) {
        std::cout << "Working...\n";
        std::this_thread::sleep_for(1s);
    }
}

int main() {
    std::thread worker(work);

    // Wait for the user to press Enter
    std::cin.get();

    finished = true;
    worker.join();

    std::cout << "Main thread id " << std::this_thread::get_id() << std::endl;

    // Keep the terminal open so you can see the output
    std::cin.get();
}