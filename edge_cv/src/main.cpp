#include "pipeline.hpp"

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

namespace {
std::atomic<bool> g_stop{false};

void signal_handler(int) {
    g_stop = true;
}
}

int main(int argc, char** argv) {
    using namespace edge_cv;

    std::string source = "0";               // default camera
    std::string model  = "models/yolo11n.onnx";

    if (argc >= 2) source = argv[1];
    if (argc >= 3) model  = argv[2];

    std::cout << "edge_cv\n"
              << "src    : " << source << "\n"
              << "model  : " << model  << "\n\n";

    Pipeline::Config cfg;
    cfg.source              = source;
    cfg.detector.model_path = model;
    cfg.detector.input_width  = 640;
    cfg.detector.input_height = 640;
    cfg.detector.conf_thresh  = 0.35f;
    cfg.detector.iou_thresh   = 0.45f;
    cfg.detector.use_cuda     = true;
    cfg.detector.num_threads  = 4;
    cfg.queue_capacity        = 2;      // only newest frames
    cfg.print_latency         = true;

    // mock watchlist
    cfg.watchlist = {
        {"person", 0.55f},
        {"car",    0.60f},
    };

    cfg.on_alert = [](const Alert& a) {
        if (a.watchlist_hit) {
            // pub to grpc
        }
    };

    try {
        Pipeline pipeline(std::move(cfg));

        std::signal(SIGINT,  signal_handler);
        std::signal(SIGTERM, signal_handler);

        pipeline.start();
        std::cout << "pipeline up..\n";

        while (!g_stop && pipeline.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        std::cout << "\nshutting down...\n";
        pipeline.stop();
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
