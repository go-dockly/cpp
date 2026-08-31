add_executable(nats_publisher
  main.cpp
  publisher_service.cpp
)
target_link_libraries(nats_publisher PRIVATE balanz_common)
