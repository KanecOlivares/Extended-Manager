cmake_minimum_required(VERSION 3.20)
project(Project1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(main
  main.cpp
)

# (Optional) show headers in IDEs without compiling them
# target_sources(main PRIVATE process.h)

# Warnings
if (MSVC)
  target_compile_options(main PRIVATE /W4 /permissive-)
else()
  target_compile_options(main PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Sanitizers (Debug only, GCC/Clang)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(main PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>)
  target_link_options(main    PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>)
endif()


# cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
# cmake --build build
# ./build/main