include_directories(${PROJECT_SOURCE_DIR}/include)

link_directories(
    ${PROJECT_SOURCE_DIR}/lib/boost
    ${PROJECT_SOURCE_DIR}/lib/spdlog
    ${PROJECT_SOURCE_DIR}/lib/openssl
)

file(GLOB_RECURSE libdco_src "*.cpp" "*.cc" "*.c")

add_library(dco SHARED ${libdco_src})

target_link_libraries(dco
    libboost_context.a
    libspdlog.a
    libssl.a
    libcrypto.a
)
