# CMAKE_CURRENT_LIST_DIR cmake 2.6 compatibility
if(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 6)
    get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
endif(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 6)

set(examples_SOURCES ${examples_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/server.c
)

if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthreads")
    set(src_DEFINITIONS ${src_DEFINITIONS} -D_WIN32_WINNT=0x0501)
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--large-address-aware") # strip all debug information
elseif(APPLE)
    set(src_DEFINITIONS ${src_DEFINITIONS} -D_REENTRANT) # enable thread safe code
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

set(examples_LIBRARIES ${CMAKE_THREADS_LIBS_INIT} -L../build -lcsnippets)

