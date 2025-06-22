cmake_minimum_required(VERSION 3.10)
project(ufmt VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(include)

# Header-only library target
add_library(ufmt INTERFACE)
target_include_directories(ufmt INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(ufmt INTERFACE cxx_std_11)

# Enable testing
enable_testing()

# Helper for cross-platform warning flags
if (MSVC)
    set(UFMT_WARNINGS /W4)
else()
    set(UFMT_WARNINGS -Wall -Wextra)
endif()

# Enable comprehensive warning flags to match Windows behavior
if(MSVC)
    # Windows (MSVC) - Enable most warnings including conversion warnings
    add_compile_options(/W4 /WX- /wd4996)
else()
    # Linux/GCC or Clang - Enable comprehensive warnings including conversion warnings
    add_compile_options(
        -Wall                    # Enable most warnings
        -Wextra                  # Enable extra warnings
        -Wconversion             # Warn about type conversions that may alter values
        -Wsign-conversion        # Warn about sign conversions
        -Wfloat-conversion       # Warn about float conversions
        -Wcast-qual              # Warn about casts that remove qualifiers
        -Wcast-align             # Warn about casts that increase alignment requirements
        -Wshadow                 # Warn about variable shadowing
        -Wpedantic               # Warn about non-standard C++
        -Wunused                 # Warn about unused variables/functions
        -Wuninitialized          # Warn about uninitialized variables
    )
endif()

# Test executable
add_executable(test_ufmt tests/test_ufmt.cpp)
target_link_libraries(test_ufmt ufmt)
target_compile_options(test_ufmt PRIVATE ${UFMT_WARNINGS})

# Add test to CTest
add_test(NAME ufmt_tests COMMAND test_ufmt)

# Demo executable
add_executable(demo_basic demos/demo_basic.cpp)
target_link_libraries(demo_basic ufmt)
target_compile_options(demo_basic PRIVATE ${UFMT_WARNINGS})

# Multi-threading demo executable
add_executable(demo_multithreading demos/demo_multithreading.cpp)
target_link_libraries(demo_multithreading ufmt)
target_compile_options(demo_multithreading PRIVATE ${UFMT_WARNINGS})
if(NOT MSVC)
    target_link_libraries(demo_multithreading pthread)
endif()

# Benchmark executable
add_executable(benchmark_basic demos/benchmark_basic.cpp)
target_link_libraries(benchmark_basic ufmt)
target_compile_options(benchmark_basic PRIVATE ${UFMT_WARNINGS} -O2)

# Multi-threading benchmark executable
add_executable(benchmark_multithreading demos/benchmark_multithreading.cpp)
target_link_libraries(benchmark_multithreading ufmt)
target_compile_options(benchmark_multithreading PRIVATE ${UFMT_WARNINGS} -O2)
if(NOT MSVC)
    target_link_libraries(benchmark_multithreading pthread)
endif()

# Multi-threading tests
add_executable(test_multithreading tests/test_multithreading.cpp)
target_link_libraries(test_multithreading ufmt)
target_compile_options(test_multithreading PRIVATE ${UFMT_WARNINGS})
if(NOT MSVC)
    target_link_libraries(test_multithreading pthread)
endif()

# Add multi-threading test to CTest
add_test(NAME ufmt_multithreading_tests COMMAND test_multithreading)

# Custom targets for convenience
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_ufmt test_multithreading
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(run_demo
    COMMAND demo_basic
    DEPENDS demo_basic
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(run_multithreading_demo
    COMMAND demo_multithreading
    DEPENDS demo_multithreading
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(run_benchmarks
    COMMAND benchmark_basic
    DEPENDS benchmark_basic
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(run_multithreading_benchmarks
    COMMAND benchmark_multithreading
    DEPENDS benchmark_multithreading
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Installation (optional)
install(TARGETS ufmt
    EXPORT ufmtTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT ufmtTargets
    FILE ufmtTargets.cmake
    NAMESPACE ufmt::
    DESTINATION lib/cmake/ufmt
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ufmtConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/ufmtConfigVersion.cmake
    DESTINATION lib/cmake/ufmt
)

# Demo: Transparent API demonstration
add_executable(demo_transparent_api demos/demo_transparent_api.cpp)
target_link_libraries(demo_transparent_api ufmt)
target_compile_options(demo_transparent_api PRIVATE ${UFMT_WARNINGS})
if(NOT MSVC)
    target_link_libraries(demo_transparent_api pthread)
endif()

# ustr integration demo executable (optional - requires ustr.h)
add_executable(demo_ustr_integration demos/demo_ustr_integration.cpp)
target_link_libraries(demo_ustr_integration ufmt)
target_compile_options(demo_ustr_integration PRIVATE ${UFMT_WARNINGS})
