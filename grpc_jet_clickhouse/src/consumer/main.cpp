#include <csignal>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <nats.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "common/env.hpp"

namespace {
std::atomic<bool> g_running{true};
void on_signal(int) { g_running = false; }
}

int main() {
  spdlog::set_level(spdlog::level::info);

  const std::string nats_url =
      balanz::getenv_or("NATS_URL", "nats://localhost:4222");
  constexpr const char* kStream = "METER_DATA";
  constexpr const char* kConsumer = "demo-consumer";
  constexpr const char* kSubject = "meter.reading";

  natsConnection* nc = nullptr;
  natsOptions* opts = nullptr;
  natsOptions_Create(&opts);
  natsOptions_SetURL(opts, nats_url.c_str());
  natsOptions_SetName(opts, "balanz-meter-consumer");
  natsOptions_SetMaxReconnect(opts, -1);
  natsOptions_SetReconnectWait(opts, 2000);

  natsStatus s = natsConnection_Connect(&nc, opts);
  natsOptions_Destroy(opts);
  if (s != NATS_OK) {
    spdlog::error("nats.Connect: {}", natsStatus_GetText(s));
    return 1;
  }

  jsCtx* js = nullptr;
  jsOptions jsOpts;
  jsOptions_Init(&jsOpts);
  s = natsConnection_JetStream(&js, nc, &jsOpts);
  if (s != NATS_OK) {
    spdlog::error("jetstream.New: {}", natsStatus_GetText(s));
    natsConnection_Destroy(nc);
    return 1;
  }

  // Wait for stream from publisher
  jsStreamInfo* si = nullptr;
  jsErrCode jerr{};
  for (int i = 0; i < 60; ++i) {
    s = js_GetStreamInfo(&si, js, kStream, nullptr, &jerr);
    if (s == NATS_OK) {
      spdlog::info("stream \"{}\" found", kStream);
      break;
    }
    spdlog::info("waiting for stream \"{}\" ... ({})", kStream, i + 1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  if (si) jsStreamInfo_Destroy(si);
  if (s != NATS_OK) {
    spdlog::error("stream {} not found: {} (jerr={})", kStream,
                  natsStatus_GetText(s), static_cast<int>(jerr));
    jsCtx_Destroy(js);
    natsConnection_Destroy(nc);
    return 1;
  }

  // Create / update consumer
  jsConsumerConfig cfg;
  jsConsumerConfig_Init(&cfg);
  cfg.Durable = const_cast<char*>(kConsumer);
  cfg.AckPolicy = js_AckExplicit;
  cfg.FilterSubject = const_cast<char*>(kSubject);
  cfg.DeliverPolicy = js_DeliverNew;

  jsConsumerInfo* ci = nullptr;
  s = js_AddConsumer(&ci, js, kStream, &cfg, nullptr, &jerr);
  if (s != NATS_OK) {
    // if already exists try update
    s = js_UpdateConsumer(&ci, js, kStream, &cfg, nullptr, &jerr);
  }
  if (ci) jsConsumerInfo_Destroy(ci);
  if (s != NATS_OK) {
    spdlog::warn("Add/UpdateConsumer: {} (jerr={}) – will try PullSubscribe anyway",
                 natsStatus_GetText(s), static_cast<int>(jerr));
  } else {
    spdlog::info("consumer \"{}\" ready on stream \"{}\"", kConsumer, kStream);
  }

  // Bind pull subscription
  jsSubOptions so;
  jsSubOptions_Init(&so);
  so.Stream = const_cast<char*>(kStream);
  so.Consumer = const_cast<char*>(kConsumer);
  so.ManualAck = true;

  natsSubscription* sub = nullptr;
  // subject can be NULL when binding
  s = js_PullSubscribe(&sub, js, nullptr, nullptr, &jsOpts, &so, &jerr);
  if (s != NATS_OK) {
    spdlog::warn("bind PullSubscribe failed: {} (jerr={})",
                 natsStatus_GetText(s), static_cast<int>(jerr));
    return 1;
  }
  if (s != NATS_OK) {
    spdlog::error("PullSubscribe: {} (jerr={})", natsStatus_GetText(s),
                  static_cast<int>(jerr));
    jsCtx_Destroy(js);
    natsConnection_Destroy(nc);
    return 1;
  }

  spdlog::info("consuming from stream \"{}\" subject {} ...", kStream, kSubject);

  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);

  while (g_running) {
    natsMsgList list{};
    s = natsSubscription_Fetch(&list, sub, 1 /*batch*/, 2000 /*ms*/, &jerr);
    if (s == NATS_TIMEOUT) continue;
    if (s != NATS_OK) {
      spdlog::error("Fetch: {} (jerr={})", natsStatus_GetText(s),
                    static_cast<int>(jerr));
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }

    for (int i = 0; i < list.Count; ++i) {
      natsMsg* msg = list.Msgs[i];
      try {
        auto payload = nlohmann::json::parse(
            std::string(natsMsg_GetData(msg), natsMsg_GetDataLength(msg)));

        uint64_t seq = 0;
        jsMsgMetaData* meta = nullptr;
        if (natsMsg_GetMetaData(&meta, msg) == NATS_OK && meta) {
          seq = meta->Sequence.Stream;
          jsMsgMetaData_Destroy(meta);
        }

        std::cout << "\n─── Meter Reading ──────────────────────────────\n"
                  << "  seq        : " << seq << "\n"
                  << "  asset_id   : " << payload.value("asset_id", "") << "\n"
                  << "  meter_id   : " << payload.value("meter_id", "") << "\n"
                  << "  power_kw   : " << payload.value("active_power_kw", 0.0)
                  << "\n"
                  << "  energy_kwh : " << payload.value("energy_kwh", 0.0)
                  << "\n"
                  << "  timestamp  : " << payload.value("timestamp", "")
                  << "\n"
                  << "────────────────────────────────────────────────\n";
      } catch (const std::exception& e) {
        spdlog::error("bad payload: {}", e.what());
      }
      natsMsg_Ack(msg, nullptr);
    }
    natsMsgList_Destroy(&list);
  }

  spdlog::info("consumer stopped");
  natsSubscription_Destroy(sub);
  jsCtx_Destroy(js);
  natsConnection_Destroy(nc);
  return 0;
}
