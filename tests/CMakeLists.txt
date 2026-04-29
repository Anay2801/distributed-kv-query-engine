add_executable(kv_tests
    smoke_test.cpp
    kv_store_test.cpp
    logged_kv_store_test.cpp
    wal_store_test.cpp
    query_test.cpp
    shard_test.cpp
    bench_test.cpp
    cli_test.cpp
)

target_include_directories(kv_tests PRIVATE ${CMAKE_SOURCE_DIR}/cli)

target_link_libraries(kv_tests
    PRIVATE
    kv_engine_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(kv_tests)
