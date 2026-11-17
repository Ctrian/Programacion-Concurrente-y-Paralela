#ifndef PALETTE_H
#define PALETTE_H

#include <vector>
#include <cstdint>

// Debe coincidir con el número de entradas definido en palette.cpp.
#define PALETTE_SIZE 16
// Declaración compartida: la única definición del vector está en palette.cpp.
extern std::vector<uint32_t> color_ramp;

#endif
