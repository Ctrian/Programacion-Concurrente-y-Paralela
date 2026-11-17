cmake_minimum_required(VERSION 3.16)

project(FractalJulia)

# Habilita instrucciones AVX2 con GCC/Clang para los archivos C++ del proyecto.
# El modo SIMD requiere una CPU compatible; no hay detección automática.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

# Compila y enlaza las tres implementaciones, la interfaz SFML y la paleta.
add_executable(fractal-julia
main.cpp
fractal-serial.cpp
palette.cpp
fractal-simd.cpp
)

target_link_libraries(fractal-julia PRIVATE 
    fmt::fmt
    SFML::Graphics SFML::Window SFML::System
)