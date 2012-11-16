cmake_minimum_required(VERSION 2.6)
project(csnippets)

set(VERSION "0.1")

include(src/CMakeLists.txt)
include(examples/CMakeLists.txt)

add_definitions(-D"VERSION=\\"${VERSION}\\"")
add_library(csnippets ${src_SOURCES})

target_link_libraries(csnippets ${src_LIBRARIES})

add_executable(server ${examples_SOURCES})
add_custom_target(run COMMAND server DEPENDS server WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
target_link_libraries(server ${examples_LIBRARIES})

