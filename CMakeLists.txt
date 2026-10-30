cmake_minimum_required(VERSION 3.20)

project(
    hft_backtester
    VERSION 0.1.0
    LANGUAGES CXX
)

# TODO 1: Enforce C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TODO 2: Disable compiler-specific language extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

# TODO 3: Add an interface target for shared compiler warnings.
add_library(project_warnings INTERFACE)

if(MSVC)
    target_compile_options(project_warnings INTERFACE /W4 /WX)
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

# TODO 3: Add the MappedFile implementation to the core library.
add_library(hft_core
    src/data/mapped_file.cpp
)
# Using PUBLIC means anything that links hft_core automatically gets this include directory
target_include_directories(hft_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hft_core PRIVATE project_warnings)


# ==============================================================================
# EXECUTABLES
# ==============================================================================
# TODO 4: Create the main executable from src/main.cpp.
add_executable(main_app src/main.cpp)
# Link the core library to the main app
target_link_libraries(main_app PRIVATE hft_core project_warnings)

# TODO 5: Enable CTest.
enable_testing()

# Find Google Test (Required to link GTest targets)
find_package(GTest REQUIRED)

# TODO 6: Create the smoke-test executable.
add_executable(smoke_test tests/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE project_warnings)

# Create the mapped_file test executable
add_executable(mapped_file_test tests/mapped_file_test.cpp)

# Fix for undefined references to Google Test macros, main(), and MappedFile!
target_link_libraries(mapped_file_test 
    PRIVATE 
    hft_core             # Links your MappedFile implementation
    GTest::gtest         # Links Google Test core
    GTest::gtest_main    # Provides the main() function for the test
    project_warnings
)

# TODO 7: Register tests with add_test().
add_test(NAME SmokeTest COMMAND smoke_test)
add_test(NAME MappedFileTest COMMAND mapped_file_test)