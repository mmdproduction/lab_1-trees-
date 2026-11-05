cmake_minimum_required(VERSION 3.14)

project(AVLTreeLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(avl_tree INTERFACE)
target_include_directories(avl_tree INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/AVL"
)

add_subdirectory(extern/googletest)

add_executable(avl_console console.cpp)
target_link_libraries(avl_console PRIVATE avl_tree)
add_executable(avl_bench benchmarks/benchmarks.cpp)
target_link_libraries(avl_bench PRIVATE avl_tree)

enable_testing()

add_executable(avl_tree_tests
    tests/AVLTreeTests.cpp
)

target_link_libraries(avl_tree_tests PRIVATE
    avl_tree
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(avl_tree_tests)
