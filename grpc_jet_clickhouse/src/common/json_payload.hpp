#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "meter/v1/meter.pb.h"

namespace balanz {

// Serialize MeterReading
inline std::string reading_to_json(const meter::v1::MeterReading& r) {
  nlohmann::json j;
  j["asset_id"]        = r.asset_id();
  j["meter_id"]        = r.meter_id();
  j["active_power_kw"] = r.active_power_kw();
  j["energy_kwh"]      = r.energy_kwh();

  if (r.has_timestamp()) {
    // RFC3339Nano
    const auto& ts = r.timestamp();
    // seconds + nanos → ISO-8601 with Z
    char buf[64];
    // UTC format
    time_t sec = static_cast<time_t>(ts.seconds());
    struct tm tm{};
    gmtime_r(&sec, &tm);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    std::string iso = buf;
    if (ts.nanos() > 0) {
      char nano[16];
      snprintf(nano, sizeof(nano), ".%09d", ts.nanos());
      iso += nano;
    }
    iso += "Z";
    j["timestamp"] = iso;
  } else {
    j["timestamp"] = nullptr;
  }
  nlohmann::json labels = nlohmann::json::object();
  for (const auto& [k, v] : r.labels()) {
    labels[k] = v;
  }
  j["labels"] = labels;

  return j.dump();
}

}
