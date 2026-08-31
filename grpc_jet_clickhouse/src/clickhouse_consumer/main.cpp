#include <csignal>
#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <thread>

#include <nats.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <clickhouse/client.h>
#include <clickhouse/columns/date.h>
#include <clickhouse/columns/numeric.h>
#include <clickhouse/columns/string.h>

#include "common/env.hpp"

namespace {

std::atomic<bool> g_running{true};
void on_signal(int) { g_running = false; }

struct Reading {
  std::string asset_id;
  std::string meter_id;
  double active_power_kw = 0;
  double energy_kwh = 0;
  std::string timestamp;
  std::map<std::string, std::string> labels;
};

Reading parse_reading(const nlohmann::json& j) {
  Reading r;
  r.asset_id = j.value("asset_id", "");
  r.meter_id = j.value("meter_id", "");
  r.active_power_kw = j.value("active_power_kw", 0.0);
  r.energy_kwh = j.value("energy_kwh", 0.0);
  r.timestamp = j.value("timestamp", "");
  if (j.contains("labels") && j["labels"].is_object()) {
    for (auto& [k, v] : j["labels"].items()) {
      r.labels[k] = v.get<std::string>();
    }
  }
  return r;
}

time_t parse_ts(const std::string& iso) {
  if (iso.empty()) return time(nullptr);
  struct tm tm{};
  std::string s = iso;
  auto dot = s.find('.');
  if (dot != std::string::npos) s = s.substr(0, dot) + "Z";
  if (strptime(s.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm) == nullptr) {
    return time(nullptr);
  }
  return timegm(&tm);
}

}

int main() {
  spdlog::set_level(spdlog::level::info);

  const std::string nats_url =
      balanz::getenv_or("NATS_URL", "nats://localhost:4222");
  const std::string ch_host =
      balanz::getenv_or("CLICKHOUSE_HOST", "localhost");
  const int ch_port =
      std::stoi(balanz::getenv_or("CLICKHOUSE_PORT", "9000"));
  const std::string ch_user =
      balanz::getenv_or("CLICKHOUSE_USER", "default");
  const std::string ch_pass =
      balanz::getenv_or("CLICKHOUSE_PASSWORD", "pass");
  const std::string ch_db =
      balanz::getenv_or("CLICKHOUSE_DB", "default");

  constexpr const char* kStream = "METER_DATA";
  constexpr const char* kConsumer = "clickhouse-consumer";
  constexpr const char* kSubject = "meter.reading";

  clickhouse::ClientOptions opts;
  opts.SetHost(ch_host);
  opts.SetPort(ch_port);
  opts.SetUser(ch_user);
  opts.SetPassword(ch_pass);
  opts.SetDefaultDatabase(ch_db);

  std::unique_ptr<clickhouse::Client> ch;
  try {
    ch = std::make_unique<clickhouse::Client>(opts);
    ch->Execute(R"(
      CREATE TABLE IF NOT EXISTS meter_readings (
          asset_id         String,
          meter_id         String,
          active_power_kw  Float64,
          energy_kwh       Float64,
          ts               DateTime64(9, 'UTC'),
          nats_seq         UInt64,
          ingested_at      DateTime64(3, 'UTC') DEFAULT now64(3),
          labels_json      String
      ) ENGINE = MergeTree()
      ORDER BY (asset_id, ts)
      TTL toDateTime(ts) + INTERVAL 90 DAY
    )");
    spdlog::info("ClickHouse table ready");
  } catch (const std::exception& e) {
    spdlog::error("ClickHouse init: {}", e.what());
    return 1;
  }

  natsConnection* nc = nullptr;
  natsOptions* nopts = nullptr;
  natsOptions_Create(&nopts);
  natsOptions_SetURL(nopts, nats_url.c_str());
  natsOptions_SetName(nopts, "balanz-clickhouse-consumer");
  natsOptions_SetMaxReconnect(nopts, -1);
  natsOptions_SetReconnectWait(nopts, 2000);

  natsStatus s = natsConnection_Connect(&nc, nopts);
  natsOptions_Destroy(nopts);
  if (s != NATS_OK) {
    spdlog::error("nats.Connect: {}", natsStatus_GetText(s));
    return 1;
  }

  jsCtx* js = nullptr;
  jsOptions jsOpts;
  jsOptions_Init(&jsOpts);
  s = natsConnection_JetStream(&js, nc, &jsOpts);
  if (s != NATS_OK) {
    spdlog::error("jetstream: {}", natsStatus_GetText(s));
    return 1;
  }

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
    spdlog::error("stream not ready: {} (jerr={})", natsStatus_GetText(s),
                  static_cast<int>(jerr));
    return 1;
  }

  jsConsumerConfig cfg;
  jsConsumerConfig_Init(&cfg);
  cfg.Durable = const_cast<char*>(kConsumer);
  cfg.AckPolicy = js_AckExplicit;
  cfg.FilterSubject = const_cast<char*>(kSubject);
  cfg.DeliverPolicy = js_DeliverNew;

  jsConsumerInfo* ci = nullptr;
  s = js_AddConsumer(&ci, js, kStream, &cfg, nullptr, &jerr);
  if (s != NATS_OK) {
    s = js_UpdateConsumer(&ci, js, kStream, &cfg, nullptr, &jerr);
  }
  if (ci) jsConsumerInfo_Destroy(ci);
  if (s != NATS_OK) {
    spdlog::warn("Add/UpdateConsumer: {} (jerr={})", natsStatus_GetText(s),
                 static_cast<int>(jerr));
  } else {
    spdlog::info("consumer \"{}\" ready", kConsumer);
  }

  jsSubOptions so;
  jsSubOptions_Init(&so);
  so.Stream = const_cast<char*>(kStream);
  so.Consumer = const_cast<char*>(kConsumer);
  so.ManualAck = true;

  natsSubscription* sub = nullptr;
  s = js_PullSubscribe(&sub, js, nullptr, nullptr, &jsOpts, &so, &jerr);
  if (s != NATS_OK) {
    spdlog::warn("bind PullSubscribe failed: {} (jerr={}), trying create path",
                 natsStatus_GetText(s), static_cast<int>(jerr));
    jsSubOptions_Init(&so);
    so.Stream = const_cast<char*>(kStream);
    so.ManualAck = true;
    so.Config.AckPolicy = js_AckExplicit;
    so.Config.FilterSubject = const_cast<char*>(kSubject);
    so.Config.DeliverPolicy = js_DeliverNew;
    s = js_PullSubscribe(&sub, js, kSubject, kConsumer, &jsOpts, &so, &jerr);
  }
  if (s != NATS_OK) {
    spdlog::error("PullSubscribe: {} (jerr={})", natsStatus_GetText(s),
                  static_cast<int>(jerr));
    return 1;
  }

  spdlog::info("clickhouse consumer ready (stream={}, consumer={})", kStream,
               kConsumer);

  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);

  while (g_running) {
    natsMsgList list{};
    s = natsSubscription_Fetch(&list, sub, 1, 2000, &jerr);
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
        auto j = nlohmann::json::parse(
            std::string(natsMsg_GetData(msg), natsMsg_GetDataLength(msg)));
        Reading r = parse_reading(j);

        uint64_t seq = 0;
        jsMsgMetaData* meta = nullptr;
        if (natsMsg_GetMetaData(&meta, msg) == NATS_OK && meta) {
          seq = meta->Sequence.Stream;
          jsMsgMetaData_Destroy(meta);
        }

        nlohmann::json labels_json = nlohmann::json::object();
        for (const auto& [k, v] : r.labels) {
          labels_json[k] = v;
        }

        clickhouse::Block block;
        auto col_asset = std::make_shared<clickhouse::ColumnString>();
        auto col_meter = std::make_shared<clickhouse::ColumnString>();
        auto col_power = std::make_shared<clickhouse::ColumnFloat64>();
        auto col_energy = std::make_shared<clickhouse::ColumnFloat64>();
        auto col_ts = std::make_shared<clickhouse::ColumnDateTime64>(9);
        auto col_seq = std::make_shared<clickhouse::ColumnUInt64>();
        auto col_labels = std::make_shared<clickhouse::ColumnString>();

        col_asset->Append(r.asset_id);
        col_meter->Append(r.meter_id);
        col_power->Append(r.active_power_kw);
        col_energy->Append(r.energy_kwh);
        col_ts->Append(static_cast<int64_t>(parse_ts(r.timestamp)) *
                       1'000'000'000LL);
        col_seq->Append(seq);
        col_labels->Append(labels_json.dump());

        block.AppendColumn("asset_id", col_asset);
        block.AppendColumn("meter_id", col_meter);
        block.AppendColumn("active_power_kw", col_power);
        block.AppendColumn("energy_kwh", col_energy);
        block.AppendColumn("ts", col_ts);
        block.AppendColumn("nats_seq", col_seq);
        block.AppendColumn("labels_json", col_labels);

        ch->Insert("meter_readings", block);

        spdlog::info("inserted asset={} power={:.2f} seq={}", r.asset_id,
                     r.active_power_kw, seq);
        natsMsg_Ack(msg, nullptr);
      } catch (const std::exception& e) {
        spdlog::error("process/insert failed: {}", e.what());
        natsMsg_Nak(msg, nullptr);
      }
    }
    natsMsgList_Destroy(&list);
  }

  spdlog::info("shutting down clickhouse consumer");
  natsSubscription_Destroy(sub);
  jsCtx_Destroy(js);
  natsConnection_Destroy(nc);
  return 0;
}
