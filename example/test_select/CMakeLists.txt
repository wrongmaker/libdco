include_directories(${PROJECT_SOURCE_DIR}/include)

link_directories(
    ${PROJECT_SOURCE_DIR}/lib/jemalloc
    ${PROJECT_BINARY_DIR}/src
)

add_executable(test_select main.cpp)

# libdco 需要在 boost之前
target_link_libraries(test_select
    libdco.so
    libjemalloc.a
)
