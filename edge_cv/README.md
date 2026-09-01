# Edge CV
low latency computer vision pipeline

1. **Ingest** openCV `VideoCapture` usb cam idx / .mp4
2. **Path** capture thread feeds drop-old queue. Frames move via `std::move` into processing thread.
3. **Inference** onnx config: letterbox + CHW preprocess, NMS inside detector
4. **Post-process** class filter (person / vehicle), confidence floor, in-mem watchlist correlation. `Alert`:{ bounding box, confidence, timestamp,  e2e latency }

## Requirements

- CMake ≥ 3.20
- C++20 compiler

```bash
brew install opencv onnxruntime
# NVIDIA pkgs on Jetson
```

### Model

```bash
./scripts/download_model.sh
```

## Build

```bash
mkdir build && cd build

cmake .. -DOpenCV_DIR=/opt/homebrew/Cellar/opencv/5.0.0_5 -DONNXRUNTIME_ROOT=/opt/homebrew/Cellar/onnxruntime/1.29.0_2 -DCMAKE_BUILD_TYPE=Release
make -j
```

## Run

```bash
# usb cam 0
./edge_cv 0 models/yolo11n.onnx

# Video file
./edge_cv /path/to/video.mp4 models/yolo11n.onnx
```
![Demo](demo.png)