#include "fractal-serial.h"
#include "palette.h"
#include "fractal-simd.h"

#include <immintrin.h> // Intrínsecos AVX: vectores de 256 bits con ocho float.
#include <complex>

// Comparte la única definición de c, ubicada en fractal-serial.cpp.
extern std::complex<double> c;
// Límite definido en main.cpp; iter limita los pasos del grupo y mk cuenta por píxel.
extern int max_iteraciones;

void julia_simd(
    double x_min, double y_min,
    double x_max, double y_max,
    uint32_t width, uint32_t height,
    uint32_t *pixel_buffer)
{
    // Se suponen dimensiones positivas y buffer de width * height elementos.
    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;

    // set1 replica un escalar en ocho carriles; aquí double se convierte a float.
    __m256 xmin = _mm256_set1_ps(x_min);

    // Equivale a set1 al repetir el mismo valor; xmin1 no se usa en el cálculo.
    __m256 xmin1 = _mm256_set_ps(
        x_min, x_min, x_min, x_min,
        x_min, x_min, x_min, x_min);

    // Borde superior del plano, replicado para calcular las ocho coordenadas y.
    __m256 ymax = _mm256_set1_ps(y_max);

    // Escalas para los ejes X e Y
    __m256 xscale = _mm256_set1_ps(dx);
    __m256 yscale = _mm256_set1_ps(dy);

    __m256 c_real = _mm256_set1_ps(c.real());
    __m256 c_imag = _mm256_set1_ps(c.imag());

    // Se compara el módulo al cuadrado con 4, evitando la raíz cuadrada.
    __m256 max_norma = _mm256_set1_ps(4.0);

    __m256 one = _mm256_set1_ps(1.0);

    // Cada grupo abarca ocho FILAS de una misma columna, no ocho columnas.
    for (int i = 0; i < width; i++)
    {

        for (int j = 0; j < height; j += 8)
        {

            __m256 mx = _mm256_set1_ps(i);

            // setr conserva el orden: carril 0 -> fila j, carril 7 -> fila j+7.
            __m256 my = _mm256_setr_ps(
                j + 0, j + 1, j + 2, j + 3,
                j + 4, j + 5, j + 6, j + 7);

            // x = x_min + i*dx es igual en los ocho carriles de esta columna.
            __m256 cr = _mm256_add_ps(
                xmin,
                _mm256_mul_ps(mx, xscale));

            // y = y_max - (j+carril)*dy cambia por fila y crece hacia arriba.
            __m256 ci = _mm256_sub_ps(
                ymax,
                _mm256_mul_ps(my, yscale));

            // Cada carril representa una órbita distinta, con sus zr y zi.

            int iter = 1;

            // mk contiene ocho contadores float iniciados en 1. A diferencia de
            // iter, solo suma en los carriles cuya comparación sea verdadera.
            __m256 mk = _mm256_set1_ps(iter);

            __m256 zr = cr;
            __m256 zi = ci;

            // En esta versión primero se actualiza z y después se comprueba
            // la norma; difiere del orden de comprobación de los modos seriales.
            while (iter < max_iteraciones)
            {

                __m256 zr2 = _mm256_mul_ps(zr, zr);  // Parte real anterior al cuadrado.
                __m256 zi2 = _mm256_mul_ps(zi, zi);  // Parte imaginaria anterior al cuadrado.
                __m256 zrzi = _mm256_mul_ps(zr, zi); // Producto de las componentes anteriores.

                // Parte real nueva: zr² - zi² + Re(c). zrzi ya conserva
                // el producto de las componentes anteriores para calcular zi.
                zr = _mm256_add_ps(_mm256_sub_ps(zr2, zi2), c_real);

                // Parte imaginaria nueva: 2*zr_anterior*zi_anterior + Im(c).
                zi = _mm256_add_ps(_mm256_add_ps(zrzi, zrzi), c_imag);

                // Módulo al cuadrado de los valores recién actualizados.
                zr2 = _mm256_mul_ps(zr, zr);             // zr² nuevo.
                zi2 = _mm256_mul_ps(zi, zi);             // zi² nuevo.
                __m256 norma2 = _mm256_add_ps(zr2, zi2); // zr² + zi².

                // LE significa <=: cada comparación verdadera genera todos los
                // bits a 1; las falsas (incluido NaN) generan todos los bits a 0.
                __m256 mask = _mm256_cmp_ps(norma2, max_norma, _CMP_LE_OQ); // NORMA <=4.0

                // AND con 1.0f produce 1.0f o 0.0f por carril para sumar a mk.
                // La máscara se recalcula: no recuerda escapes previos ni congela
                // las órbitas que ya escaparon; todos los zr/zi siguen avanzando.
                mk = _mm256_add_ps(mk, _mm256_and_ps(mask, one));
                // testz comprueba bits (no compara float): con máscaras de todos
                // unos o ceros, da verdadero si ningún carril cumple norma² <= 4.
                // set1_ps(-1) replica -1.0f; no crea una máscara de todos unos.
                if (_mm256_testz_ps(mask, _mm256_set1_ps(-1)))
                {
                    break;
                }

                iter++;
            }

            // Copiar los ocho contadores a memoria sin exigir alineación de 32 bytes.
            float d[8];
            _mm256_storeu_ps(d, mk);
            /*
             * d[0] -> contador del píxel (columna i, fila j).
             * ...
             * d[7] -> contador del píxel (columna i, fila j+7).
             * */

            /*
             * [(j+0)*width+i]
             * ...
             * [(j+7)*width+i] */
            for (int it = 0; it < 8; it++)
            {
                int index = (j + it) * width + i;

                // Si height no es múltiplo de ocho, solo se escriben las filas
                // válidas. Los carriles sobrantes sí se calculan en el while.
                if (index < width * height)
                {
                    if (d[it] < max_iteraciones)
                    {
                        int color_index = (int)d[it] % PALETTE_SIZE;
                        pixel_buffer[index] = color_ramp[color_index];
                    }
                    else
                    {
                        pixel_buffer[index] = 0xFF000000; // Negro opaco en x86 (RGBA).
                    }
                }
            }
        }
    }
}