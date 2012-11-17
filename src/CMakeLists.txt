# CMAKE_CURRENT_LIST_DIR cmake 2.6 compatibility
if(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 6)
    get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
endif(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 6)

set(src_LIBRARIES "")
set(src_DEFINITIONS "")
set(src_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../csnippets")
set(src_SOURCES ${src_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/asprintf.c
    ${CMAKE_CURRENT_LIST_DIR}/config.c
    ${CMAKE_CURRENT_LIST_DIR}/event.c
    ${CMAKE_CURRENT_LIST_DIR}/map.c
    ${CMAKE_CURRENT_LIST_DIR}/socket.c
    ${CMAKE_CURRENT_LIST_DIR}/socket_select.c
    ${CMAKE_CURRENT_LIST_DIR}/socket_epoll.c
    ${CMAKE_CURRENT_LIST_DIR}/strmisc.c
    ${CMAKE_CURRENT_LIST_DIR}/error.c
    ${CMAKE_CURRENT_LIST_DIR}/list.c
    ${CMAKE_CURRENT_LIST_DIR}/rwlock.c
    ${CMAKE_CURRENT_LIST_DIR}/task.c
)

set(BUILD_COMMIT "devel" CACHE "Git commit string (intended for releases)" STRING)
set(BUILD_REVISION "0" CACHE "Git revision string (intended for releases)" STRING)

# default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

# gcc compile flags
set(WARNS_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-variable")
set(CMAKE_C_FLAGS                   "${CMAKE_C_FLAGS} ${WARNS_FLAGS} ${ARCH_FLAGS} -pipe -include ${CMAKE_CURRENT_LIST_DIR}/../csnippets/csnippets.h")
set(CMAKE_C_FLAGS_DEBUG             "-O0 -g")
set(CMAKE_C_FLAGS_RELWITHDEBINFO    "-O1 -g -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_RELEASE           "-O2")
set(CMAKE_C_FLAGS_PERFORMANCE       "-Ofast -mmmx -msse -msse2")
set(CMAKE_C_FLAGS_MINSIZEREL        "-Os")

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
add_definitions(-D"BUILD_TYPE=\\\"${CMAKE_BUILD_TYPE}\\\"")
message(STATUS "Build commit: ${BUILD_COMMIT}")
add_definitions(-D"BUILD_COMMIT=\\\"${BUILD_COMMIT}\\\"")
message(STATUS "Build revision: ${BUILD_REVISION}")
add_definitions(-D"BUILD_REVISION=\\\"${BUILD_REVISION}\\\"")

if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthreads -D__use_select")
    set(src_DEFINITIONS ${src_DEFINITIONS} -D_WIN32_WINNT=0x0501)
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--large-address-aware") # strip all debug information
    set(src_LIBRARIES "-lws2_32")
elseif(APPLE)
    set(src_DEFINITIONS ${src_DEFINITIONS} -D_REENTRANT) # enable thread safe code
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "Debug information: ON")
else()
    message(STATUS "Debug information: OFF")
    set(src_DEFINITIONS ${src_DEFINITIONS} -DNDEBUG) # NDEBUG disable asserts
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--as-needed -Wl,-s") # strip all debug information
endif()

include_directories(${src_INCLUDE_DIRS})
add_definitions(${src_DEFINITIONS})

