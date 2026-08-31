add_executable(ingest_server
  main.cpp
  ingest_service.cpp
)
target_link_libraries(ingest_server PRIVATE balanz_common)
