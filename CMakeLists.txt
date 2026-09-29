cmake_minimum_required(VERSION 3.20)

project(net_diagnostics_cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4)
    # Keep the bounded standard-C calls portable across compilers.
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(netdiag_core STATIC
        src/cli.c
        src/json.c
        src/dns.c
        src/scan.c
        src/ping.c
        src/report.c
)
target_include_directories(netdiag_core PUBLIC src)
add_executable(netdiag src/main.c)
target_link_libraries(netdiag PRIVATE netdiag_core)

if (WIN32)
    target_link_libraries(netdiag_core PUBLIC ws2_32)
else()
    target_compile_definitions(netdiag_core PUBLIC _POSIX_C_SOURCE=200112L)
endif()

include(CTest)
if (BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(test_ping tests/test_ping.c)
    target_link_libraries(test_ping PRIVATE netdiag_core)
    add_test(NAME ping_summary_fixtures COMMAND test_ping "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_executable(test_scan_errors tests/test_scan_errors.c)
    target_link_libraries(test_scan_errors PRIVATE netdiag_core)
    add_test(NAME scan_error_classification COMMAND test_scan_errors)
    add_executable(netdiag_fixture src/main.c src/cli.c src/json.c src/report.c tests/stub_network.c)
    target_include_directories(netdiag_fixture PRIVATE src)
    add_test(NAME cli_contract COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.py" "$<TARGET_FILE:netdiag_fixture>")
    add_test(NAME local_endpoints COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_endpoints.py" "$<TARGET_FILE:netdiag>")
    set_tests_properties(cli_contract local_endpoints PROPERTIES TIMEOUT 45)
endif()
