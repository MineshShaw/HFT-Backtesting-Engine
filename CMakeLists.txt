cmake_minimum_required(VERSION 3.20)

project(
    hft_backtester
    VERSION 0.1.0
    LANGUAGES CXX
)

# Enforce C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable compiler-specific language extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

# Add an interface target for shared compiler warnings.
add_library(project_warnings INTERFACE)

if(MSVC)
    target_compile_options(project_warnings INTERFACE /W4 /WX)
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(hft_core 
    src/data/mapped_file.cpp 
    src/data/csv_parser.cpp
    src/data/numeric_converter.cpp
    src/data/tick_converter.cpp
)
# Using PUBLIC means anything that links hft_core automatically gets this include directory
target_include_directories(hft_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hft_core PRIVATE project_warnings)


# Create the main executable from src/main.cpp.
add_executable(main_app src/main.cpp)
# Link the core library to the main app
target_link_libraries(main_app PRIVATE hft_core project_warnings)

# Enable CTest.
enable_testing()

# Find Google Test (Required to link GTest targets)
find_package(GTest REQUIRED)

add_executable(smoke_test tests/smoke_test.cpp)
add_executable(mapped_file_test tests/mapped_file_test.cpp)
add_executable(csv_parser_test tests/csv_parser_test.cpp)
add_executable(numeric_converter_test tests/numeric_converter_test.cpp)
add_executable(tick_converter_test tests/tick_converter_test.cpp)

target_link_libraries(smoke_test PRIVATE project_warnings)

target_link_libraries(mapped_file_test 
    PRIVATE 
    hft_core             # Links your MappedFile implementation
    GTest::gtest         # Links Google Test core
    GTest::gtest_main    # Provides the main() function for the test
    project_warnings
)

target_link_libraries(csv_parser_test
    PRIVATE
    hft_core             # Links your CsvParser implementation
    GTest::gtest         # Links Google Test core
    GTest::gtest_main    # Provides the main() function for the test
    project_warnings
)

target_link_libraries(numeric_converter_test
    PRIVATE
    hft_core             # Links your NumericConverter implementation
    GTest::gtest         # Links Google Test core
    GTest::gtest_main    # Provides the main() function for the test
    project_warnings
)

target_link_libraries(tick_converter_test
    PRIVATE
    hft_core
    GTest::gtest
    GTest::gtest_main
    project_warnings
)

add_test(NAME SmokeTest COMMAND smoke_test)
add_test(NAME MappedFileTest COMMAND mapped_file_test)
add_test(NAME CsvParserTest COMMAND csv_parser_test)
add_test(NAME NumericConverterTest COMMAND numeric_converter_test)
add_test(NAME TickConverterTest COMMAND tick_converter_test)