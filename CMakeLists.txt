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

# TODO 4: Create the main executable from src/main.cpp.
add_executable(main_app src/main.cpp)
target_link_libraries(main_app PRIVATE project_warnings)

# TODO 5: Enable CTest.
enable_testing()

# TODO 6: Create the smoke-test executable.
add_executable(smoke_test test/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE project_warnings)

# TODO 7: Register the smoke test with add_test().
add_test(NAME SmokeTest COMMAND smoke_test)
