#include "fractal-serial.h"
#include "palette.h"

#include <complex> // std::complex<double> representa x + y*i.

// Parámetro fijo del conjunto de Julia: todos los píxeles comparten c.
// Esta es su única definición; SIMD accede al mismo valor mediante extern.
std::complex<double> c(-0.7, 0.27015);
// Se define en main.cpp y cambia con las teclas Up/Down.
extern int max_iteraciones;


// Versión Serial I: calcula el color de una órbita usando std::complex.
uint32_t acotado(std::complex<double> z0)
{
    /*
     * Cada píxel aporta su propio z0; se repite z(n+1) = z(n)^2 + c.
     * El límite permite observar una órbita finita, no demostrar pertenencia.
     */

    // Con un límite positivo se hacen como máximo max_iteraciones - 1 pasos.
    int iter = 1;

    std::complex<double> z = z0;

    // Se comprueba el módulo ANTES de actualizar: |z| >= 2 detiene la órbita.
    while (iter < max_iteraciones && std::abs(z) < 2.0)
    {
        z = z * z + c;
        iter++;
    }
    if (iter < max_iteraciones)
    {
        // Escape detectado antes del límite; el módulo % repite la paleta.
        int index = iter % PALETTE_SIZE;
        return color_ramp[index];
    }
    // Se alcanzó el límite, incluso si el último paso terminó fuera del radio.
    // En x86 little-endian son bytes RGBA FF 00 00 00: rojo con alfa 0.
    return 0X000000FF;
}

void julia_serial(
    double x_min, double y_min, double x_max, double y_max,
    uint32_t width, uint32_t height,
    uint32_t *pixel_buffer)
{
    // Paso entre muestras; se supone width > 0 y height > 0.
    // La última columna no alcanza x_max y la última fila no alcanza y_min.
    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            double x = x_min + i * dx;
            // Las filas crecen hacia abajo; el eje imaginario, hacia arriba.
            double y = y_max - j * dy;
            // Punto inicial independiente para este píxel.
            std::complex<double> z0(x, y);

            uint32_t color = acotado(z0);

            // Buffer por filas: saltar j filas completas y avanzar i columnas.
            pixel_buffer[j * width + i] = color;
        }
    }
}

// Sobrecarga para Serial II: recibe las dos componentes en lugar de un complejo.
uint32_t acotado(double x, double y)
{
    /*
     * Cada píxel aporta su propio z0; se repite z(n+1) = z(n)^2 + c.
     * El límite permite observar una órbita finita, no demostrar pertenencia.
     */

    // Con un límite positivo se hacen como máximo max_iteraciones - 1 pasos.
    int iter = 1;

    double zr = x;
    double zi = y;

    // |z|² < 4 equivale a |z| < 2 y evita calcular la raíz cuadrada.
    while (iter < max_iteraciones && (zr*zr+zi*zi) < 4.0)
    {
        // (zr + zi*i)² = (zr² - zi²) + (2*zr*zi)*i.
        // Ambas componentes nuevas deben usar zr y zi ANTERIORES.
        double new_zr = zr*zr-zi*zi + c.real();
        double new_zi = 2*zr*zi + c.imag();

        zr = new_zr;
        zi = new_zi;
        iter++;
    }
    if (iter < max_iteraciones)
    {
        // Escape detectado antes del límite; el módulo % repite la paleta.
        int index = iter % PALETTE_SIZE;
        return color_ramp[index];
    }
    // Se alcanzó el límite, incluso si el último paso terminó fuera del radio.
    // En x86 little-endian son bytes RGBA FF 00 00 00: rojo con alfa 0.
    return 0X000000FF;
}

// Mismo recorrido de píxeles que Serial I, con aritmética real explícita.
void julia_serial_2(
    double x_min, double y_min, double x_max, double y_max,
    uint32_t width, uint32_t height,
    uint32_t *pixel_buffer)
{
    // Paso entre muestras; se supone width > 0 y height > 0.
    // La última columna no alcanza x_max y la última fila no alcanza y_min.
    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            double x = x_min + i * dx;
            // Las filas crecen hacia abajo; el eje imaginario, hacia arriba.
            double y = y_max - j * dy;
            // Los dos double seleccionan la sobrecarga de acotado para Serial II.
            uint32_t color = acotado(x, y);

            // Buffer por filas: saltar j filas completas y avanzar i columnas.
            pixel_buffer[j * width + i] = color;
        }
    }
}