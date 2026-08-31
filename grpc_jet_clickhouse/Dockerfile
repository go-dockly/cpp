FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git pkg-config curl ca-certificates \
    libprotobuf-dev protobuf-compiler \
    libgrpc++-dev libgrpc-dev protobuf-compiler-grpc \
    libssl-dev zlib1g-dev \
    libabsl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# layer shared by all compose services
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_CLICKHOUSE_CONSUMER=ON \
 && cmake --build build -j$(nproc) \
 && echo "=== built binaries ===" \
 && find build -type f -executable \( -name 'nats_publisher' -o -name 'ingest_server' \
      -o -name 'client' -o -name 'consumer' -o -name 'clickhouse_consumer' \) | sort

FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libprotobuf32t64 \
    libgrpc++1.51t64 \
    libgrpc29t64 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/src/nats_publisher/nats_publisher           /usr/local/bin/nats_publisher
COPY --from=builder /src/build/src/ingest/ingest_server                    /usr/local/bin/ingest_server
COPY --from=builder /src/build/src/client/client                           /usr/local/bin/client
COPY --from=builder /src/build/src/consumer/consumer                       /usr/local/bin/consumer
COPY --from=builder /src/build/src/clickhouse_consumer/clickhouse_consumer /usr/local/bin/clickhouse_consumer

ARG TARGET=nats_publisher
ENV SERVICE_BIN=${TARGET}
ENTRYPOINT ["/bin/sh", "-c", "exec /usr/local/bin/${SERVICE_BIN}"]
