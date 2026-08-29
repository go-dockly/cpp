#include <iostream>
#include <future>
#include <chrono>
#include <thread>
#include <string>

// func that takes some time to run
std::string long_running_task(int seconds) {
    std::cout << "Starting task_long (" << seconds << " s)...\n";
    
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    
    std::cout << "task_long finished!\n";
    return "Result from async task_long";
}

int main() {
    std::cout << "Launching async task...\n";

    std::future<std::string> result = std::async(
        std::launch::async,
        long_running_task,
        3                       // in seconds
    );

    std::cout << "Do other work while task_long runs...\n";
    for (int i = 1; i <= 5; ++i) {
        std::cout << "Working... " << i << "/5\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    std::cout << "Waiting for async result...\n";
    std::string value = result.get();

    std::cout << "Received: \"" << value << "\"\n";
    std::cin.get();
    return 0;
}