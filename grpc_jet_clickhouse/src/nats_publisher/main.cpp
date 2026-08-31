#include <csignal>
#include <memory>
#include <string>
#include <chrono>

#include <grpcpp/grpcpp.h>
#include <nats.h>
#include <spdlog/spdlog.h>

#include "common/env.hpp"
#include "publisher_service.hpp"

using namespace std::chrono_literals;

namespace {

std::unique_ptr<grpc::Server> g_server;
natsConnection* g_nc = nullptr;
jsCtx* g_js = nullptr;

void signal_handler(int) {
  spdlog::info("shutting down...");
  if (g_server) g_server->Shutdown();
}

}

int main() {
  spdlog::set_level(spdlog::level::info);

  const std::string nats_url =
      balanz::getenv_or("NATS_URL", "nats://localhost:4222");
  const std::string listen_addr =
      balanz::getenv_or("GRPC_ADDR", "0.0.0.0:50051");

  // NATS connection
  natsOptions* opts = nullptr;
  natsOptions_Create(&opts);
  natsOptions_SetURL(opts, nats_url.c_str());
  natsOptions_SetName(opts, "balanz-nats-publisher");
  natsOptions_SetMaxReconnect(opts, -1);
  natsOptions_SetReconnectWait(opts, 2000);

  natsStatus s = natsConnection_Connect(&g_nc, opts);
  natsOptions_Destroy(opts);
  if (s != NATS_OK) {
    spdlog::error("nats connect: {}", natsStatus_GetText(s));
    return 1;
  }

  jsOptions jsOpts;
  jsOptions_Init(&jsOpts);
  s = natsConnection_JetStream(&g_js, g_nc, &jsOpts);
  if (s != NATS_OK) {
    spdlog::error("jetstream: {}", natsStatus_GetText(s));
    natsConnection_Destroy(g_nc);
    return 1;
  }

  // Create or update stream
  jsStreamConfig sc;
  jsStreamConfig_Init(&sc);
  sc.Name = const_cast<char*>("METER_DATA");
  const char* subjects[] = {"meter.>"};
  sc.Subjects = subjects;
  sc.SubjectsLen = 1;
  sc.Retention = js_LimitsPolicy;
  sc.MaxAge = std::chrono::nanoseconds(24h).count();
  sc.Storage = js_FileStorage;
  sc.Replicas = 1;

  jsStreamInfo* si = nullptr;
  jsErrCode jerr{};
  s = js_AddStream(&si, g_js, &sc, nullptr, &jerr);
  if (s != NATS_OK) {
    // try update
    s = js_UpdateStream(&si, g_js, &sc, nullptr, &jerr);
  }
  if (si) jsStreamInfo_Destroy(si);

  if (s != NATS_OK) {
    spdlog::warn("create/update stream: {} (jerr={}) – continuing",
                 natsStatus_GetText(s), static_cast<int>(jerr));
  } else {
    spdlog::info("JetStream stream \"METER_DATA\" ready");
  }

  // gRPC server
  balanz::NatsPublisherServiceImpl service(g_js);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(listen_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  g_server = builder.BuildAndStart();
  if (!g_server) {
    spdlog::error("failed to start gRPC server on {}", listen_addr);
    return 1;
  }
  spdlog::info("NATS Publisher gRPC listening on {}", listen_addr);

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  g_server->Wait();

  if (g_js) jsCtx_Destroy(g_js);
  if (g_nc) natsConnection_Destroy(g_nc);
  return 0;
}
