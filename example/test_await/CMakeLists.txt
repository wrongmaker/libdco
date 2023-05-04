include_directories(${PROJECT_SOURCE_DIR}/include)

link_directories(
    ${PROJECT_SOURCE_DIR}/lib/jemalloc
    ${PROJECT_BINARY_DIR}/src
)

# add_library(libdco SHARED IMPORTED)

add_executable(test_await main.cpp)

# libdco 需要在 boost之前
target_link_libraries(test_await
    libdco.so
    libjemalloc.a
)
