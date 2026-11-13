cmake_minimum_required(VERSION 3.16)
project(SmartSortingStudio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(smart_sorting_studio
    src/main.cpp
)

if (MSVC)
    target_compile_options(smart_sorting_studio PRIVATE /W4 /EHsc)
else()
    target_compile_options(smart_sorting_studio PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    target_link_libraries(smart_sorting_studio PRIVATE ws2_32)
endif()
