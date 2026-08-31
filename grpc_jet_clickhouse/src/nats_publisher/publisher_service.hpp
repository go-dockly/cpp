#pragma once

#include <memory>
#include <string>

#include <nats.h>

#include "nats/v1/publisher_service.grpc.pb.h"

namespace balanz {

class NatsPublisherServiceImpl final : public nats::v1::NatsPublisherService::Service {
 public:
  explicit NatsPublisherServiceImpl(jsCtx* js);
  ~NatsPublisherServiceImpl() override = default;

  grpc::Status PublishMeterReading(
      grpc::ServerContext* context,
      const nats::v1::PublishMeterReadingRequest* request,
      nats::v1::PublishMeterReadingResponse* response) override;

 private:
  jsCtx* js_;  // non-owning; owned by the process
  static constexpr const char* kDefaultSubject = "meter.reading";
};

}
