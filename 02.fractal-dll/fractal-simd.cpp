#include "palette.h"
#include "fractal-simd.h"

#include <immintrin.h> // Intrínsecos para vectores de 256 bits.
#include <complex>

#include<iostream>

// Parámetro común de Julia: cada píxel varía z0, pero usa el mismo c.
std::complex<double> c(-0.7, 0.27015);

void julia_simd(
    double x_min, double y_min,
    double x_max, double y_max,
    uint32_t width, uint32_t height,
    int max_iteraciones,
    uint32_t *pixel_buffer)
{
    // Traza de diagnóstico: imprime y fuerza la salida en cada llamada.
    // Ese trabajo de consola también influye si se mide el tiempo de la función.
    printf("*******************************************julia_simd()\n");
    fflush(stdout);

    // Separación entre muestras. No se incluyen x_max ni y_min en la imagen.
    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;

    // __m256 contiene ocho float, no ocho números complejos completos.
    // set1 replica un escalar; al pasar de double a float se reduce la precisión.
    __m256 xmin = _mm256_set1_ps(x_min);

    // Demostración equivalente a xmin mediante set_ps; xmin1 no se usa después.
    __m256 xmin1 = _mm256_set_ps(
        x_min, x_min, x_min, x_min,
        x_min, x_min, x_min, x_min);

    // Límite superior replicado para convertir índices de fila en coordenadas y.
    __m256 ymax = _mm256_set1_ps(y_max);

    // Escalas para los ejes X e Y
    __m256 xscale = _mm256_set1_ps(dx);
    __m256 yscale = _mm256_set1_ps(dy);

    // Partes real e imaginaria de c replicadas en todos los carriles.
    __m256 c_real = _mm256_set1_ps(c.real());
    __m256 c_imag = _mm256_set1_ps(c.imag());

    // |z|² = x² + y²: comparar con 4 evita calcular la raíz cuadrada.
    __m256 max_norma = _mm256_set1_ps(4.0);

    __m256 one = _mm256_set1_ps(1.0);

    // i identifica una columna; j avanza ocho FILAS de esa misma columna.
    // Cada carril mantiene una órbita independiente: no son ocho pasos de z.
    for (int i = 0; i < width; i++)
    {

        for (int j = 0; j < height; j += 8)
        {

            // La parte real inicial es igual para los ocho píxeles del grupo.
            __m256 mx = _mm256_set1_ps(i);

            // setr conserva el orden: el carril 0 corresponde a la fila j.
            __m256 my = _mm256_setr_ps(
                j + 0, j + 1, j + 2, j + 3,
                j + 4, j + 5, j + 6, j + 7);

            // x = x_min + i * dx, repetido en los ocho carriles.
            __m256 cr = _mm256_add_ps(
                xmin,
                _mm256_mul_ps(mx, xscale));

            // y = y_max - (j + carril) * dy: las filas crecen hacia abajo.
            __m256 ci = _mm256_sub_ps(
                ymax,
                _mm256_mul_ps(my, yscale));

            // iter limita los pasos del grupo; mk guarda un contador por píxel.
            // Empezar en 1 permite como máximo max_iteraciones - 1 pasos.
            int iter = 1;

            // Contadores float: los enteros dejan de ser exactos por encima de 2^24.
            __m256 mk = _mm256_set1_ps(iter);

            __m256 zr = cr;
            __m256 zi = ci;

            while (iter < max_iteraciones)
            {

                // Guardar x², y² y xy antes de modificar las componentes de z.
                __m256 zr2 = _mm256_mul_ps(zr, zr);
                __m256 zi2 = _mm256_mul_ps(zi, zi);
                __m256 zrzi = _mm256_mul_ps(zr, zi);

                // z_nuevo = z_anterior² + c: x_nuevo = x² - y² + Re(c).
                zr = _mm256_add_ps(_mm256_sub_ps(zr2, zi2), c_real);

                // y_nuevo = 2*x*y + Im(c), usando el producto ANTERIOR zrzi.
                zi = _mm256_add_ps(_mm256_add_ps(zrzi, zrzi), c_imag);

                // Esta versión comprueba el módulo DESPUÉS de actualizar z.
                zr2 = _mm256_mul_ps(zr, zr);
                zi2 = _mm256_mul_ps(zi, zi);
                __m256 norma2 = _mm256_add_ps(zr2, zi2);

                // LE_OQ comprueba <= 4, con resultado falso si hay NaN. Cada
                // carril verdadero contiene bits 0xFFFFFFFF; el falso, 0x00000000.
                __m256 mask = _mm256_cmp_ps(norma2, max_norma, _CMP_LE_OQ);

                // AND selecciona 1.0f o 0.0f: sumar solo si la comparación pasó.
                // La máscara se recalcula en cada paso; no conserva escapes previos.
                mk = _mm256_add_ps(mk, _mm256_and_ps(mask, one));
                // Terminar cuando ninguna comparación del grupo es verdadera.
                if (_mm256_testz_ps(mask, _mm256_set1_ps(-1)))
                {
                    break;
                }

                iter++;
            }

            // Pasar los ocho contadores a un arreglo sin exigir alineación especial.
            float d[8];
            _mm256_storeu_ps(d, mk);
            /*
             * d[0] corresponde al píxel de columna i y fila j.
             * d[7] corresponde al píxel de columna i y fila j+7.
             * Estos contadores suman comparaciones verdaderas tras actualizar z;
             * no equivalen exactamente al contador de la versión serial que
             * comprueba |z| < 2 antes de actualizar. Igualarlas es trabajo pendiente.
             * */

            // Memoria por filas: cada resultado se separa del siguiente por width.
            // Los ocho colores calculados no son contiguos en el buffer.
            for (int it = 0; it < 8; it++)
            {
                int index = (j + it) * width + i;

                // Si faltan filas para completar el grupo, omitir sus escrituras.
                // El cálculo SIMD anterior sí incluyó esos carriles adicionales.
                if (index < width * height)
                {
                    if (d[it] < max_iteraciones)
                    {
                        // El módulo repite los 16 colores de la paleta.
                        int color_index = (int)d[it] % PALETTE_SIZE;
                        pixel_buffer[index] = color_ramp[color_index];
                    }
                    else
                    {
                        pixel_buffer[index] = 0xFF000000; // Negro opaco: RGBA 00 00 00 FF en x86.
                    }
                }
            }
        }
    }
}
