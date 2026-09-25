cmake_minimum_required(VERSION 3.20)
project(cosmos_x1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

add_library(ncm
    src/Config.cpp
    src/Matrix.cpp
    src/CharacterCodebook.cpp
    src/Viewer.cpp
)
target_include_directories(ncm PUBLIC include)
target_link_libraries(ncm PUBLIC OpenMP::OpenMP_CXX)

add_executable(cosmos_x1 src/main.cpp)
target_link_libraries(cosmos_x1 PRIVATE ncm)
