#ifndef FRACTAL_SERIAL_H
#define FRACTAL_SERIAL_H

// Protección de inclusión: evita procesar dos veces este encabezado en una
// misma unidad de compilación. No controla las iteraciones del fractal.
#include <cstdint>

// Serial I: función libre que calcula cada píxel con std::complex<double>.
// [x_min, x_max] e [y_min, y_max]: región del plano complejo que observamos.
// width y height: resolución de la imagen; se supone que ambos son > 0.
// pixel_buffer: memoria ya reservada para al menos width * height colores.
// No devuelve la imagen: escribe sus colores directamente en esa memoria.
void julia_serial(
    double x_min, double y_min, double x_max, double y_max,
    uint32_t width, uint32_t height,
    uint32_t*pixel_buffer
);

// Serial II: mismo contrato, con partes real e imaginaria en double separados.
// Comprueba el módulo al cuadrado y actualiza z sin usar std::complex.
void julia_serial_2(
    double x_min, double y_min, double x_max, double y_max,
    uint32_t width, uint32_t height,
    uint32_t*pixel_buffer
);

#endif // FRACTAL_SERIAL_H
