cmake_minimum_required(VERSION 3.20)

project(net_diagnostics_cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable more warnings for better code quality
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(netdiag
        src/main.c
        src/dns.h
        src/dns.c
)
