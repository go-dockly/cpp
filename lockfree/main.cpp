#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstring>

// lock-free Single-Producer Single-Consumer (SPSC) ring buffer
//    one writer (disk thread), one reader (audio)
template<typename T, size_t Capacity>
class LockFreeRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    alignas(64) std::atomic<size_t> writePos{0};   // written by producer
    alignas(64) std::atomic<size_t> readPos{0};    // written by consumer
    T buffer[Capacity];

public:
    bool push(const T& item) {
        const size_t w = writePos.load(std::memory_order_relaxed);
        const size_t next = (w + 1) & (Capacity - 1);
        if (next == readPos.load(std::memory_order_acquire))
            return false;                       // full
        buffer[w] = item;
        writePos.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        const size_t r = readPos.load(std::memory_order_relaxed);
        if (r == writePos.load(std::memory_order_acquire))
            return false; // empty
        item = buffer[r];
        readPos.store((r + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    size_t size_approx() const {
        return (writePos.load(std::memory_order_relaxed) -
                readPos.load(std::memory_order_relaxed)) & (Capacity - 1);
    }
};

// Audio params GUI can change during runtime
struct AudioParams {
    std::atomic<float> attack {0.010f};   // seconds
    std::atomic<float> decay  {1.250f};
    std::atomic<float> sustain{0.670f};
    std::atomic<float> release{0.220f};
    std::atomic<float> level  {0.190f};
};

// Shared state
constexpr size_t RING_SIZE = 4096;          // power of 2
LockFreeRingBuffer<float, RING_SIZE> audioRing;
AudioParams params;

std::atomic<bool> running{true};

// Disk / file-reading thread  (producer)
// Simulate audio reading loop from disk
void diskReaderThread() {
    float phase = 0.0f;
    const float freq = 440.0f;
    const float sampleRate = 44100.0f;
    const float twoPi = 6.28318530718f;

    while (running.load(std::memory_order_relaxed)) {
        // Gen simple sine
        float sample = std::sin(phase) * 0.3f;
        phase += twoPi * freq / sampleRate;
        if (phase > twoPi) phase -= twoPi;

        // Push into lock-free ring.  If full wait a tiny bit
        // should never happen as audio consumes fast
        while (!audioRing.push(sample) && running.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    }
}

// Audio callback  (runs on high-prio audio thread)
// Func must NEVER block or lock
// ------------------------------------------------------------------
void audioCallback(float* output, size_t numFrames) {
    // Snapshot params once per buffer (cheap atomic loads)
    const float A = params.attack.load(std::memory_order_relaxed);
    const float D = params.decay.load(std::memory_order_relaxed);
    const float S = params.sustain.load(std::memory_order_relaxed);
    const float R = params.release.load(std::memory_order_relaxed);
    const float L = params.level.load(std::memory_order_relaxed);

    for (size_t i = 0; i < numFrames; ++i) {
        float sample = 0.0f;

        // Lock-free read from ring buffer
        if (!audioRing.pop(sample)) {
            // output silence
            sample = 0.0f;
        }
        // Apply current level
        output[i] = sample * L;
    }
}

// Simulated audio thread (eg OS audio callback)
void audioThread() {
    constexpr size_t bufferSize = 256;
    float output[bufferSize];

    while (running.load(std::memory_order_relaxed)) {
        audioCallback(output, bufferSize);

        // Simulate the time it takes to play 256 samples @ 44.1 kHz
        std::this_thread::sleep_for(std::chrono::microseconds(5800));
    }
}

void guiThread() {
    using namespace std::chrono_literals;

    std::this_thread::sleep_for(1s);

    std::cout << "[GUI] Changing envelope parameters...\n";
    params.attack  = 0.005f;
    params.decay   = 0.800f;
    params.sustain = 0.450f;
    params.release = 0.350f;
    params.level   = 0.85f;

    std::this_thread::sleep_for(2s);

    std::cout << "[GUI] Lowering level...\n";
    params.level = 0.25f;

    std::this_thread::sleep_for(2s);
    running = false;
}

// ------------------------------------------------------------------
// main
// ------------------------------------------------------------------
int main() {
    std::cout << "Starting lock-free audio demo...\n";
    std::cout << " - Disk thread fills ring buffer\n";
    std::cout << " - Audio thread reads it with zero locks\n";
    std::cout << " - GUI thread changes params via atomics\n\n";

    std::thread disk(diskReaderThread);
    std::thread audio(audioThread);
    std::thread gui(guiThread);

    gui.join();
    audio.join();
    disk.join();

    std::cout << "\nShutdown. Samples remaining in ring_buffer: "
              << audioRing.size_approx() << '\n';

    std::cin.get();
    return 0;
}