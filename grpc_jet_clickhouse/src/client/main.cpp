#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/time_util.h>
#include <spdlog/spdlog.h>

#include "common/env.hpp"
#include "meter/v1/ingest_service.grpc.pb.h"

int main() {
  spdlog::set_level(spdlog::level::info);

  const std::string addr =
      balanz::getenv_or("INGEST_ADDR", "localhost:50052");

  auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
  auto stub = meter::v1::IngestService::NewStub(channel);

  const std::vector<std::string> assets = {
      "NL-SOLAR-001", "NL-SOLAR-042", "NL-WIND-007", "NL-BESS-003"};

  std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<size_t> asset_dist(0, assets.size() - 1);
  std::uniform_real_distribution<double> power_dist(50.0, 250.0);

  for (int i = 0; i < 5; ++i) {
    const std::string& asset = assets[asset_dist(rng)];
    const double power = power_dist(rng);
    const double energy = power * 0.25;

    meter::v1::IngestReadingRequest req;
    auto* r = req.mutable_reading();
    r->set_asset_id(asset);
    r->set_meter_id("M-" + asset.substr(asset.size() - 3));
    r->set_active_power_kw(power);
    r->set_energy_kwh(energy);
    *r->mutable_timestamp() =
        google::protobuf::util::TimeUtil::GetCurrentTime();
    (*r->mutable_labels())["region"] = "NL";
    (*r->mutable_labels())["type"] = "renewable";

    meter::v1::IngestReadingResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() +
                     std::chrono::seconds(5));

    auto status = stub->IngestReading(&ctx, req, &resp);
    if (!status.ok()) {
      spdlog::error("ingest error: {}", status.error_message());
      continue;
    }

    if (resp.accepted()) {
      std::cout << "accepted  asset=" << std::left << std::setw(14) << asset
                << " power=" << std::setw(6) << std::fixed << std::setprecision(1)
                << power << " kW  msg_id=" << resp.message_id() << "\n";
    } else {
      std::cout << "rejected  asset=" << asset;
      if (resp.has_error()) {
        std::cout << "  err=" << resp.error().message();
      }
      std::cout << "\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  return 0;
}
