cmake_minimum_required(VERSION 3.16)

project(FractalJulia)

# pasar configuracion al compilador c/c++
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

add_executable(fractal-julia
main.cpp
fractal-serial.cpp
palette.cpp
fractal-simd.cpp
fractal-simd.h)

target_link_libraries(fractal-julia PRIVATE 
    fmt::fmt
    SFML::Graphics SFML::Window SFML::System
)