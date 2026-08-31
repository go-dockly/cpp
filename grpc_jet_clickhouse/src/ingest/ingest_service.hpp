#pragma once

#include <memory>
#include <string>

#include "meter/v1/ingest_service.grpc.pb.h"
#include "nats/v1/publisher_service.grpc.pb.h"

namespace balanz {

class IngestServiceImpl final : public meter::v1::IngestService::Service {
 public:
  explicit IngestServiceImpl(
      std::shared_ptr<nats::v1::NatsPublisherService::Stub> nats_stub);

  grpc::Status IngestReading(
      grpc::ServerContext* context,
      const meter::v1::IngestReadingRequest* request,
      meter::v1::IngestReadingResponse* response) override;

 private:
  std::shared_ptr<nats::v1::NatsPublisherService::Stub> nats_stub_;
  static std::string format_seq(uint64_t seq);
};

}
