#include "fractal-simd.h"
#include "palette.h"

#include <algorithm>
#include <complex>
#include <cstddef>
#include <immintrin.h>

// c se define una sola vez en fractal-serial.cpp; ambos modos usan el mismo valor.
extern std::complex<double> c;
// El límite se define en main.cpp y cambia con las teclas Up/Down.
extern int max_iteraciones;

void julia_simd(
    double x_min, double y_min, double x_max, double y_max,
    uint32_t width, uint32_t height,
    uint32_t *pixel_buffer)
{
    if (width == 0 || height == 0)
        return;

    const double dx = (x_max - x_min) / width;
    const double dy = (y_max - y_min) / height;
    const __m256 xmin = _mm256_set1_ps(static_cast<float>(x_min));
    const __m256 ymax = _mm256_set1_ps(static_cast<float>(y_max));
    const __m256 xscale = _mm256_set1_ps(static_cast<float>(dx));
    const __m256 yscale = _mm256_set1_ps(static_cast<float>(dy));
    const __m256 c_real = _mm256_set1_ps(static_cast<float>(c.real()));
    const __m256 c_imag = _mm256_set1_ps(static_cast<float>(c.imag()));
    const __m256 max_norma = _mm256_set1_ps(4.0f);
    const __m256i lanes = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);

    // Cada grupo contiene ocho FILAS de una misma columna: x es común,
    // mientras y cambia por carril. No son ocho iteraciones de un solo píxel.
    for (std::size_t i = 0; i < width; ++i)
    {
        const __m256 mx = _mm256_set1_ps(static_cast<float>(i));
        const __m256 cr = _mm256_add_ps(xmin, _mm256_mul_ps(mx, xscale));

        for (std::size_t j = 0; j < height; j += 8)
        {
            const __m256 my = _mm256_setr_ps(
                static_cast<float>(j),     static_cast<float>(j + 1),
                static_cast<float>(j + 2), static_cast<float>(j + 3),
                static_cast<float>(j + 4), static_cast<float>(j + 5),
                static_cast<float>(j + 6), static_cast<float>(j + 7));
            __m256 zr = cr;
            __m256 zi = _mm256_sub_ps(ymax, _mm256_mul_ps(my, yscale));

            // El último grupo puede tener menos de ocho filas válidas.
            const int valid_rows = static_cast<int>(
                std::min<std::size_t>(8, height - j));
            __m256 active = _mm256_castsi256_ps(_mm256_cmpgt_epi32(
                _mm256_set1_epi32(valid_rows), lanes));
            __m256i counts = _mm256_set1_epi32(1);

            // Igual que en serial: comprobar |z| < 2 ANTES de actualizar y
            // empezar en 1 permite como máximo max_iteraciones - 1 pasos.
            for (int iter = 1; iter < max_iteraciones; ++iter)
            {
                const __m256 zr2 = _mm256_mul_ps(zr, zr);
                const __m256 zi2 = _mm256_mul_ps(zi, zi);
                const __m256 norma2 = _mm256_add_ps(zr2, zi2);
                const __m256 inside = _mm256_cmp_ps(norma2, max_norma, _CMP_LT_OQ);

                // Una vez que un carril escapa, permanece inactivo.
                active = _mm256_and_ps(active, inside);
                if (_mm256_movemask_ps(active) == 0)
                    break;

                // Ambas componentes usan los valores ANTERIORES de zr y zi.
                const __m256 zrzi = _mm256_mul_ps(zr, zi);
                const __m256 next_r = _mm256_add_ps(
                    _mm256_sub_ps(zr2, zi2), c_real);
                const __m256 next_i = _mm256_add_ps(
                    _mm256_add_ps(zrzi, zrzi), c_imag);
                zr = _mm256_blendv_ps(zr, next_r, active);
                zi = _mm256_blendv_ps(zi, next_i, active);

                // Cada máscara activa contiene -1: restarla suma un paso
                // solo al contador de ese píxel. AVX2 opera ocho int de 32 bits.
                counts = _mm256_sub_epi32(counts, _mm256_castps_si256(active));
            }

            int iterations[8];
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(iterations), counts);
            for (int lane = 0; lane < valid_rows; ++lane)
            {
                const std::size_t index = (j + lane) * width + i;
                pixel_buffer[index] = iterations[lane] < max_iteraciones
                    ? color_ramp[iterations[lane] % PALETTE_SIZE]
                    : 0xFF000000; // Bytes RGBA 00 00 00 FF en x86: negro opaco.
            }
        }
    }
}
