#include <iostream>
#include <stdexcept>
#include <string>

// resource acquired in ctor, released in dtor
class FileHandle {
    std::string name;
    bool open_ = false;
public:
    explicit FileHandle(std::string n) : name(std::move(n)) {
        // pretend open file
        open_ = true;
        std::cout << "  opened " << name << "\n";
    }
    ~FileHandle() {
        if (open_) {
            std::cout << "  closed " << name << "\n";
            open_ = false;
        }
    }
    // non-copyable (rule of 5)
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    void write(const std::string& data) {
        if (!open_) throw std::runtime_error("write on closed handle");
        std::cout << "  wrote \"" << data << "\" to " << name << "\n";
    }
};

void process(bool should_throw) {
    FileHandle f("data.txt");          // resource acquired
    f.write("hello");
    if (should_throw)
        throw std::runtime_error("something went wrong");
    f.write("world");                  // never reached if exception
}                                      // destructor always runs to close file

int main() {
    std::cout << "normal path\n";
    try {
        process(false);
    } catch (const std::exception& e) {
        std::cout << "  caught: " << e.what() << "\n";
    }

    std::cout << "exception path RAII still cleans up\n";
    try {
        process(true);
    } catch (const std::exception& e) {
        std::cout << "  caught: " << e.what() << "\n";
    }
    std::cout << "exit\n";
}