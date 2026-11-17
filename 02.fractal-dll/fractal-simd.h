#ifndef FRACTAL_SIMD_H
#define FRACTAL_SIMD_H

#include <cstdint>

// Enlace C: evita la decoración del nombre de C++ (name mangling), de modo
// que un consumidor puede buscar el símbolo "julia_simd" en la biblioteca.
// El encabezado está escrito para incluirse desde C++; no es un header C puro.
extern "C"
// Calcula el fractal y escribe sus colores en memoria reservada por el llamador.
// La biblioteca no reserva ni libera pixel_buffer ni retiene el puntero.
// x_min/x_max e y_min/y_max delimitan la región del plano complejo.
// width y height deben ser positivos; width * height y los índices deben caber
// en int. pixel_buffer debe tener espacio para al menos width * height uint32_t.
// max_iteraciones es el presupuesto por llamada; se recomienda que sea >= 2.
// Usa ocho float por vector: pueden existir diferencias frente a una versión
// escalar con double. El procesador debe soportar AVX2.
void julia_simd(
    double x_min, double y_min, double x_max, double y_max,
    uint32_t width, uint32_t height,
    int max_iteraciones,
    uint32_t*pixel_buffer
);

#endif
