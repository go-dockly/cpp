add_executable(clickhouse_consumer main.cpp)
target_link_libraries(clickhouse_consumer PRIVATE
  balanz_common
  clickhouse-cpp-lib
)
