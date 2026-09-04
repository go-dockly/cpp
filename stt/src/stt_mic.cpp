#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "portaudio.h"
#include "sherpa-onnx/c-api/cxx-api.h"

namespace {

std::atomic<bool> g_stop{false};

void on_signal(int) {
  g_stop = true;
  std::cerr << "\n[Ctrl-C] shutting down…\n";
}

void print_usage(const char* prog) {
  std::cerr
      << "Usage: " << prog << " [options]\n"
      << "  --tokens   <path>   (required)\n"
      << "  --encoder  <path>   (required)\n"
      << "  --decoder  <path>   (required)\n"
      << "  --joiner   <path>   (required)\n"
      << "  --threads  <int>    (default 2)\n"
      << "  --provider <str>   (default cpu)\n"
      << "  --help\n";
}

// portaudio callback ring-buffer of float samples
struct AudioQueue {
  std::mutex mtx;
  std::condition_variable cv;
  std::queue<std::vector<float>> q;
  bool stopped = false;

  void push(const float* data, size_t n) {
    std::lock_guard<std::mutex> lock(mtx);
    q.emplace(data, data + n);
    cv.notify_one();
  }

  // Returns false when stopped & queue empty.
  bool pop(std::vector<float>& out) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&] { return stopped || !q.empty(); });
    if (q.empty()) return false;
    out = std::move(q.front());
    q.pop();
    return true;
  }

  void stop() {
    std::lock_guard<std::mutex> lock(mtx);
    stopped = true;
    cv.notify_all();
  }
};

AudioQueue g_audio;

// portaudio callback
int pa_callback(const void* input, void* /*output*/,
                unsigned long frame_count,
                const PaStreamCallbackTimeInfo* /*time_info*/,
                PaStreamCallbackFlags /*status*/,
                void* /*user_data*/) {
  if (g_stop) return paComplete;
  const float* samples = static_cast<const float*>(input);
  g_audio.push(samples, frame_count);
  return paContinue;
}

struct Args {
  std::string tokens;
  std::string encoder;
  std::string decoder;
  std::string joiner;
  int num_threads = 2;
  std::string provider = "cpu";          // cuda | coreml
  float sample_rate = 16000.f;
  bool enable_endpoint = true;
};

Args parse_args(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto need = [&](const char* name) -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "Missing argument for " << name << "\n";
        std::exit(1);
      }
      return argv[++i];
    };
    if (arg == "--tokens")        a.tokens = need("--tokens");
    else if (arg == "--encoder")  a.encoder = need("--encoder");
    else if (arg == "--decoder")  a.decoder = need("--decoder");
    else if (arg == "--joiner")   a.joiner = need("--joiner");
    else if (arg == "--threads")  a.num_threads = std::stoi(need("--threads"));
    else if (arg == "--provider") a.provider = need("--provider");
    else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      std::exit(0);
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      std::exit(1);
    }
  }
  if (a.tokens.empty() || a.encoder.empty() || a.decoder.empty() ||
      a.joiner.empty()) {
    std::cerr << "Error: --tokens/--encoder/--decoder/--joiner are required arguments\n";
    print_usage(argv[0]);
    std::exit(1);
  }
  return a;
}

}

int main(int argc, char** argv) {
  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);

  Args args = parse_args(argc, argv);

  // streaming recognizer
  using namespace sherpa_onnx::cxx;

  OnlineRecognizerConfig config;
  config.model_config.transducer.encoder = args.encoder;
  config.model_config.transducer.decoder = args.decoder;
  config.model_config.transducer.joiner  = args.joiner;
  config.model_config.tokens             = args.tokens;
  config.model_config.num_threads        = args.num_threads;
  config.model_config.provider           = args.provider;
  config.model_config.debug              = false;

  // silence based end detection
  config.enable_endpoint = args.enable_endpoint;
  config.rule1_min_trailing_silence = 2.4f;
  config.rule2_min_trailing_silence = 1.2f;
  config.rule3_min_utterance_length = 20.f;   // frames

  std::cout << "Loading streaming model…\n";
  OnlineRecognizer recognizer = OnlineRecognizer::Create(config);
  if (!recognizer.Get()) {
    std::cerr << "Failed to create OnlineRecognizer. Check model path.\n";
    return 1;
  }
  std::cout << "Model loaded.\n";

  OnlineStream stream = recognizer.CreateStream();

  // open mic with portaudio
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    std::cerr << "Pa_Initialize failed: " << Pa_GetErrorText(err) << "\n";
    return 1;
  }

  int device = Pa_GetDefaultInputDevice();
  if (const char* env = std::getenv("SHERPA_ONNX_MIC_DEVICE")) {
    device = std::atoi(env);
  }
  if (device == paNoDevice) {
    std::cerr << "No default input device.\n";
    Pa_Terminate();
    return 1;
  }

  const PaDeviceInfo* info = Pa_GetDeviceInfo(device);
  std::cout << "Using input device #" << device << ": "
            << (info ? info->name : "unknown") << "\n";

  float mic_sr = args.sample_rate;
  if (const char* env = std::getenv("SHERPA_ONNX_MIC_SAMPLE_RATE")) {
    mic_sr = std::atof(env);
  }

  PaStreamParameters params{};
  params.device                    = device;
  params.channelCount              = 1;
  params.sampleFormat              = paFloat32;
  params.suggestedLatency          = info ? info->defaultLowInputLatency : 0.01;
  params.hostApiSpecificStreamInfo = nullptr;

  PaStream* pa_stream = nullptr;
  err = Pa_OpenStream(&pa_stream,
                      &params,
                      nullptr,               // no output
                      mic_sr,
                      1024,                  // frames per buffer
                      paClipOff,
                      pa_callback,
                      nullptr);
  if (err != paNoError) {
    std::cerr << "Pa_OpenStream failed: " << Pa_GetErrorText(err) << "\n";
    Pa_Terminate();
    return 1;
  }

  err = Pa_StartStream(pa_stream);
  if (err != paNoError) {
    std::cerr << "Pa_StartStream failed: " << Pa_GetErrorText(err) << "\n";
    Pa_CloseStream(pa_stream);
    Pa_Terminate();
    return 1;
  }

  std::cout << "\n=== Listening (Ctrl-C to stop) ===\n"
            << "Partial transcript:\n\n";

  // recognition loop
  std::string last_text;
  int segment_id = 0;

  while (!g_stop) {
    std::vector<float> samples;
    if (!g_audio.pop(samples)) break;

    // feed audio (sherpa-onnx resamples internally)
    stream.AcceptWaveform(mic_sr, samples.data(), samples.size());

    while (recognizer.IsReady(&stream)) {
      recognizer.Decode(&stream);
    }

    OnlineRecognizerResult result = recognizer.GetResult(&stream);
    std::string text = result.text;

    // print only when hypothesis changes
    if (!text.empty() && text != last_text) {
      // carriage-return overwrite
      std::cout << "\r\033[K"   // clear line
                << "[" << segment_id << "] " << text << std::flush;
      last_text = text;
    }

    if (recognizer.IsEndpoint(&stream)) {
      if (!last_text.empty()) {
        std::cout << "\n";
        ++segment_id;
      }
      recognizer.Reset(&stream);
      last_text.clear();
    }
  }

  g_audio.stop();
  Pa_StopStream(pa_stream);
  Pa_CloseStream(pa_stream);
  Pa_Terminate();

  // flush audio buffer
  stream.InputFinished();
  while (recognizer.IsReady(&stream)) {
    recognizer.Decode(&stream);
  }
  auto final_result = recognizer.GetResult(&stream);
  if (!final_result.text.empty() && final_result.text != last_text) {
    std::cout << "\n[" << segment_id << "] " << final_result.text << "\n";
  }

  std::cout << "\nDone.\n";
  return 0;
}