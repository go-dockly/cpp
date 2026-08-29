#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <memory>

constexpr int MAX_THREADS = 8;
constexpr int HAZARD_SLOTS = 1;          // one hazard ptr per thread

struct HazardRecord {
    std::atomic<void*> hazard{nullptr};
};

HazardRecord g_hazards[MAX_THREADS];     // global table of hazard ptrs
std::atomic<int> g_thread_count{0};

// thread-local idx into global hazard table
thread_local int t_id = -1;

void register_thread() {
    t_id = g_thread_count.fetch_add(1);
    if (t_id >= MAX_THREADS) {
        std::cerr << "Too many threads for this example\n";
        std::abort();
    }
}

// protect ptr (publish it so other threads won't free it)
template<typename T>
T* protect(std::atomic<T*>& src) {
    T* p;
    do {
        p = src.load(std::memory_order_acquire);
        g_hazards[t_id].hazard.store(p, std::memory_order_release);
        // re-check because the ptr could have changed between the two loads
    } while (p != src.load(std::memory_order_acquire));
    return p;
}

void clear_hazard() {
    g_hazards[t_id].hazard.store(nullptr, std::memory_order_release);
}

// lock-free stack of Entities
class Entity {
public:
    Entity(int v) : value(v) {
        std::cout << "Created Entity " << value << '\n';
    }
    ~Entity() {
        std::cout << "Destroyed Entity " << value << '\n';
    }
    void Print() const {
        std::cout << "Entity value = " << value << '\n';
    }

    int value;
    Entity* next = nullptr;          // used by stack
};

class LockFreeStack {
    std::atomic<Entity*> head{nullptr};
    std::vector<Entity*> retired;    // nodes waiting to be reclaimed
    static constexpr size_t RETIRE_THRESHOLD = 4;

public:
    void push(int value) {
        Entity* node = new Entity(value);
        Entity* old_head;
        do {
            old_head = head.load(std::memory_order_relaxed);
            node->next = old_head;
        } while (!head.compare_exchange_weak(old_head, node,
                                             std::memory_order_release,
                                             std::memory_order_relaxed));
    }

    // pop with hazard-ptr protection
    Entity* pop() {
        Entity* old_head;
        do {
            // protect head ptr
            old_head = protect(head);
            if (!old_head) {
                clear_hazard();
                return nullptr;
            }
            // try to swing head
        } while (!head.compare_exchange_weak(old_head, old_head->next,
                                             std::memory_order_acquire,
                                             std::memory_order_relaxed));

        clear_hazard();               // no longer need protection

        // defer reclamation
        retire(old_head);
        return old_head; // caller may use it
    }

private:
    void retire(Entity* node) {
        retired.push_back(node);
        if (retired.size() >= RETIRE_THRESHOLD) {
            scan_and_reclaim();
        }
    }

    void scan_and_reclaim() {
        // collect all protected ptrs
        std::vector<void*> hazards;
        for (int i = 0; i < g_thread_count.load(); ++i) {
            void* h = g_hazards[i].hazard.load(std::memory_order_acquire);
            if (h) hazards.push_back(h);
        }

        // keep only nodes that aren't protected
        std::vector<Entity*> still_retired;
        for (Entity* node : retired) {
            bool is_hazard = false;
            for (void* h : hazards) {
                if (h == node) {
                    is_hazard = true;
                    break;
                }
            }
            if (is_hazard) {
                still_retired.push_back(node);   // still in use → keep
            } else {
                delete node;                     // safe to free
            }
        }
        retired.swap(still_retired);
    }

public:
    // force final scan (before program exit)
    void force_reclaim() {
        scan_and_reclaim();
        // anything left still protected; in a real program use more sophisticated scheme
        for (Entity* n : retired) delete n;
        retired.clear();
    }
};

int main() {
    LockFreeStack stack;

    // register main thread
    register_thread();

    std::thread producer([&] {
        register_thread();
        for (int i = 0; i < 8; ++i) {
            stack.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });

    // Consumer
    std::thread consumer([&] {
        register_thread();
        for (int i = 0; i < 8; ++i) {
            Entity* e = stack.pop();
            if (e) {
                e->Print();               // safe: still protected
                // no need to delete e here - stack retires it
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    });

    producer.join();
    consumer.join();

    stack.force_reclaim();   // clean up any remaining nodes

    std::cout << "Done.\n";
    std::cin.get();
    return 0;
}