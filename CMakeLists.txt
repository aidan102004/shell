cmake_minimum_required(VERSION 3.13)

project(bird-shell-cpp)

# Set BEFORE creating the target
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

add_executable(shell ${SOURCE_FILES})

# Explicitly set on target as well
target_compile_features(shell PRIVATE cxx_std_23)

target_link_libraries(shell PRIVATE readline)