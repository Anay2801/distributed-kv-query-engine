find_package(Threads REQUIRED)

add_library(kv_engine_lib STATIC
    kv_store.cpp
    logged_kv_store.cpp
    wal_store.cpp
    consistent_hash_ring.cpp
    node_server.cpp
    node_client.cpp
    shard_router.cpp
)
target_include_directories(kv_engine_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(kv_engine_lib PUBLIC Threads::Threads)
