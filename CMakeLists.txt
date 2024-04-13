cmake_minimum_required(VERSION 3.20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(miniserver LANGUAGES CXX)

find_package(spdlog REQUIRED)
find_package(magic_enum REQUIRED)

file(GLOB SOURCES "src/*.cpp")
add_executable(miniserver ${SOURCES})
target_include_directories(miniserver PUBLIC include)
target_link_libraries(miniserver PRIVATE magic_enum::magic_enum)
target_link_libraries(miniserver PRIVATE spdlog::spdlog)
target_link_libraries(miniserver PUBLIC mysqlclient)
