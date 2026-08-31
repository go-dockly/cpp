#include "ingest_service.hpp"

#include <spdlog/spdlog.h>

#include "common/v1/error.pb.h"

namespace balanz {

IngestServiceImpl::IngestServiceImpl(
    std::shared_ptr<nats::v1::NatsPublisherService::Stub> nats_stub)
    : nats_stub_(std::move(nats_stub)) {}

std::string IngestServiceImpl::format_seq(uint64_t seq) {
  return "js-" + std::to_string(seq);
}

grpc::Status IngestServiceImpl::IngestReading(
    grpc::ServerContext* /*context*/,
    const meter::v1::IngestReadingRequest* request,
    meter::v1::IngestReadingResponse* response) {

  if (!request->has_reading()) {
    response->set_accepted(false);
    auto* err = response->mutable_error();
    err->set_code("INVALID_ARGUMENT");
    err->set_message("reading is required");
    return grpc::Status::OK;
  }

  nats::v1::PublishMeterReadingRequest pub_req;
  *pub_req.mutable_reading() = request->reading();
  pub_req.set_subject("meter.reading");

  nats::v1::PublishMeterReadingResponse pub_resp;
  grpc::ClientContext client_ctx;
  // inherit deadline if present
  auto status = nats_stub_->PublishMeterReading(&client_ctx, pub_req, &pub_resp);

  if (!status.ok()) {
    spdlog::error("call to nats publisher failed: {}", status.error_message());
    return grpc::Status(grpc::StatusCode::INTERNAL,
                        "nats publisher unavailable: " + status.error_message());
  }

  if (!pub_resp.published()) {
    std::string msg = "publish rejected";
    if (pub_resp.has_error()) {
      msg = pub_resp.error().message();
    }
    response->set_accepted(false);
    auto* err = response->mutable_error();
    err->set_code("PUBLISH_FAILED");
    err->set_message(msg);
    return grpc::Status::OK;
  }

  spdlog::info("ingested asset={} power={:.2f} kW → NATS seq={}",
               request->reading().asset_id(),
               request->reading().active_power_kw(),
               pub_resp.sequence());

  response->set_accepted(true);
  response->set_message_id(format_seq(pub_resp.sequence()));
  return grpc::Status::OK;
}

}
