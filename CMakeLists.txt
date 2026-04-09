cmake_minimum_required(VERSION 3.22)
project(featureflagx VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package(Drogon CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Collect library sources (everything in src/ except main.cpp)
file(GLOB_RECURSE LIB_SOURCES
    "src/config/*.cpp"
    "src/models/*.cpp"
    "src/repositories/*.cpp"
    "src/services/*.cpp"
    "src/controllers/*.cpp"
    "src/middleware/*.cpp"
)

# Static library for shared linking between app and tests
add_library(featureflagx_lib STATIC ${LIB_SOURCES})
target_include_directories(featureflagx_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(featureflagx_lib PUBLIC
    Drogon::Drogon
    libpqxx::pqxx
    hiredis::hiredis
    redis++::redis++
    jwt-cpp::jwt-cpp
    nlohmann_json::nlohmann_json
)

# Main executable
add_executable(featureflagx src/main.cpp)
target_link_libraries(featureflagx PRIVATE featureflagx_lib)

# Tests
enable_testing()
file(GLOB TEST_SOURCES "tests/*.cpp")
if(TEST_SOURCES)
    add_executable(featureflagx_tests ${TEST_SOURCES})
    target_link_libraries(featureflagx_tests PRIVATE
        featureflagx_lib
        GTest::gtest
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(featureflagx_tests)
endif()
