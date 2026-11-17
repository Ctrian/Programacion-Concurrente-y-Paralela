#ifndef PALETTE_H
#define PALETTE_H

#include <vector>
#include <cstdint>

// Cantidad de entradas para el ciclo de colores; debe coincidir con color_ramp.
#define PALETTE_SIZE 16
// Declaración compartida; el vector se define una sola vez en palette.cpp.
extern std::vector<uint32_t> color_ramp;

#endif