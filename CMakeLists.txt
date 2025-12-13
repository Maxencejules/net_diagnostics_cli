cmake_minimum_required(VERSION 3.20)

project(net_diagnostics_cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(netdiag
        src/main.c
        src/dns.c
        src/scan.c
        src/ping.c
)

if (WIN32)
    target_link_libraries(netdiag ws2_32)
endif()
