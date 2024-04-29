# for each "src/x.cpp", generate target "x"
file(GLOB_RECURSE all_srcs CONFIGURE_DEPENDS *.cpp)
foreach(v ${all_srcs})
    string(REGEX MATCH "src/.*" relative_path ${v})
    # message(${relative_path})
    string(REGEX REPLACE "src/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})

    add_executable(${target_name} ${v})
    # target_link_libraries(${target_name} PRIVATE ${EXTRA_LIBRARIES})
    target_compile_definitions(${target_name} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
    # target_compile_definitions(${target_name} PRIVATE  MYSQLPP_MYSQL_HEADERS_BURIED)

endforeach()