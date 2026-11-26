cmake_minimum_required(VERSION 3.16)
project(GraphLab CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_library(graph_core INTERFACE)
target_include_directories(graph_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestRunner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/regular/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/lazy/*.cpp)
add_library(graph_tests STATIC ${TEST_SOURCES})
target_include_directories(graph_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(graph_tests PUBLIC graph_core)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(GraphLab ${APP_SOURCES})
target_link_libraries(GraphLab PRIVATE graph_core graph_tests)
if(WIN32)
    target_link_libraries(GraphLab PRIVATE gdi32 user32)
endif()

add_executable(GraphTests tests/test_main.cpp)
target_link_libraries(GraphTests PRIVATE graph_tests)