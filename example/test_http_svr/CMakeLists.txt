include_directories(${PROJECT_SOURCE_DIR}/include)

link_directories(
    ${PROJECT_SOURCE_DIR}/lib/jemalloc
    ${PROJECT_BINARY_DIR}/src
    /usr/lib/x86_64-linux-gnu/
)

add_executable(test_http_svr main.cpp)

# libdco 需要在 boost之前
target_link_libraries(test_http_svr
    libdco.so
    libjemalloc.a
    libssl.a
    libcrypto.a
)
