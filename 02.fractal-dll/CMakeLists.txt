cmake_minimum_required(VERSION 3.16)

project(FractalJulia)

# Habilita intrínsecos AVX2 con GCC/Clang; requiere un procesador x86 con AVX2.
# Esta opción no detecta en ejecución si el equipo soporta esas instrucciones.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

# SHARED crea una biblioteca, no un ejecutable: aquí no se necesita main().
# En Linux genera libfractal-julia.so para cargarla desde otro programa.
add_library(fractal-julia SHARED
palette.cpp
fractal-simd.cpp
)
