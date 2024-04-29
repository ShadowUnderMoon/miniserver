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

set(EXTRA_LIBRARIES
    magic_enum::magic_enum
    spdlog::spdlog
    mysqlpp
)

# file(GLOB SOURCES "src/*.cpp")
# add_executable(miniserver ${SOURCES})
# target_include_directories(miniserver PUBLIC include)
# target_link_libraries(miniserver PRIVATE magic_enum::magic_enum)
# target_link_libraries(miniserver PRIVATE spdlog::spdlog)
# target_link_libraries(miniserver PUBLIC mysqlclient)


include_directories(${PROJECT_SOURCE_DIR}/include)
link_libraries(${EXTRA_LIBRARIES})

add_subdirectory(./src)

add_subdirectory(./test)

add_subdirectory(./example)