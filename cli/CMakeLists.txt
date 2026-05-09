add_executable(kv_cli kv_cli.cpp)
target_include_directories(kv_cli PRIVATE ${CMAKE_SOURCE_DIR}/cli)
target_link_libraries(kv_cli PRIVATE kv_engine_lib)
