add_executable(kv_bench bench_main.cpp)
target_link_libraries(kv_bench PRIVATE kv_engine_lib)
