#ifndef FRACTAL_SIMD_H
#define FRACTAL_SIMD_H

#include <cstdint>

// Calcula ocho píxeles de una misma columna con AVX2; requiere CPU x86 con AVX2.
// Usa float (ocho valores por vector de 256 bits), frente al double de serial.
// Cerca de la frontera pueden variar los colores por la precisión numérica.
// Los contadores suman según una máscara recalculada en cada paso; las órbitas
// escapadas siguen actualizándose. Se comprueba norma² <= 4 después de actualizar,
// frente a la comprobación previa y estricta < 4 de los modos seriales.
// Recibe la misma región y buffer que julia_serial; admite alturas no múltiplos
// de ocho. width y height deben ser > 0; el buffer necesita width * height elementos.
void julia_simd(
    double x_min, double y_min, double x_max, double y_max,
    uint32_t width, uint32_t height,
    uint32_t*pixel_buffer
);

#endif // FRACTAL_SIMD_H
