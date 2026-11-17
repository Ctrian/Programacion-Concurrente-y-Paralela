#include "fractal-serial.h"
#include "fractal-simd.h"
#include "palette.h"

#include <complex>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

extern std::complex<double> c;
int max_iteraciones = 10;

// Referencia escalar en float: separa errores de SIMD de las diferencias
// esperables frente al double de julia_serial, especialmente en la frontera.
static uint32_t reference(float x, float y)
{
    int iter = 1;
    while (iter < max_iteraciones && x * x + y * y < 4.0f)
    {
        const float xx = x * x;
        const float yy = y * y;
        const float xy = x * y;
        x = (xx - yy) + static_cast<float>(c.real());
        y = (xy + xy) + static_cast<float>(c.imag());
        ++iter;
    }
    return iter < max_iteraciones ? color_ramp[iter % PALETTE_SIZE] : 0xFF000000;
}

static bool check(uint32_t width, uint32_t height,
                  double xmin, double ymin, double xmax, double ymax)
{
    const uint32_t sentinel = 0x12345678;
    const std::size_t size = static_cast<std::size_t>(width) * height;
    // Los centinelas detectan escrituras fuera del buffer, incluso sin sanitizers.
    std::vector<uint32_t> pixels(size + 2, sentinel);
    julia_simd(xmin, ymin, xmax, ymax, width, height, pixels.data() + 1);
    if (pixels.front() != sentinel || pixels.back() != sentinel)
        return false;
    if (width == 0 || height == 0)
        return true;

    const float dx = static_cast<float>((xmax - xmin) / width);
    const float dy = static_cast<float>((ymax - ymin) / height);
    for (uint32_t j = 0; j < height; ++j)
    {
        for (uint32_t i = 0; i < width; ++i)
        {
            const float x = static_cast<float>(xmin) + static_cast<float>(i) * dx;
            const float y = static_cast<float>(ymax) - static_cast<float>(j) * dy;
            if (pixels[1 + static_cast<std::size_t>(j) * width + i] != reference(x, y))
            {
                std::cerr << "Diferencia en " << width << 'x' << height
                          << " pixel " << i << ',' << j
                          << " limite " << max_iteraciones << '\n';
                return false;
            }
        }
    }
    return true;
}

int main()
{
    for (int limit : {-1, 0, 1, 2, 3, 10, 100})
    {
        max_iteraciones = limit;
        for (uint32_t width : {0u, 1u, 2u, 5u, 17u})
            for (uint32_t height : {0u, 1u, 7u, 8u, 9u, 15u, 16u, 17u})
                if (!check(width, height, -1.5, -1.5, 1.5, 1.5))
                    return 1;
    }
    max_iteraciones = 100;
    if (!check(1600, 900, -1.5, -1.5, 1.5, 1.5))
        return 1;

    // Puntos sobre el radio 2 ya están fuera antes de la primera actualización.
    // Con c=-4, z=2 volvería a cero si se actualizara indebidamente.
    c = {-4.0, 0.0};
    max_iteraciones = 10;
    if (!check(3, 9, 2.0, -1.0, 3.5, 0.0))
        return 1;

    // Órbitas que escapan pronto no deben recorrer todo un límite muy grande.
    c = {0.0, 0.0};
    max_iteraciones = std::numeric_limits<int>::max();
    if (!check(3, 9, 1.5, -1.0, 3.0, 0.0))
        return 1;

    // Con coordenadas y c exactamente representables, ambos modos coinciden.
    max_iteraciones = 10;
    std::vector<uint32_t> serial(8 * 16), simd(serial.size());
    julia_serial(-2, -2, 2, 2, 8, 16, serial.data());
    julia_simd(-2, -2, 2, 2, 8, 16, simd.data());
    if (serial != simd)
    {
        std::cerr << "Diferencia entre modos con coordenadas exactas\n";
        return 1;
    }
    std::cout << "Pruebas de iteraciones, mascara, bordes y colores correctas\n";
}
