#include "palette.h"

// Paleta de colores para los contadores del fractal.
// Generador de referencia:
// https://rampgenerator.com/

// Invierte los cuatro bytes: de la notación literal RRGGBBAA a AABBGGRR.
// En x86 (little-endian), el entero resultante queda en memoria como R,G,B,A.
uint32_t bswap32(uint32_t a){
    return  ((a & 0x000000FF) << 24)|
            ((a & 0x0000FF00) << 8) |
            ((a & 0x00FF0000) >> 8) |
            ((a & 0xFF000000) >> 24);
}

// Un color por entrada; iteraciones % PALETTE_SIZE selecciona el índice.
std::vector<uint32_t> color_ramp ={

    bswap32(0xFF1010FF), 
    bswap32(0xEF1019FF),
    bswap32(0xE01123FF),
    bswap32(0xD1112DFF),
    bswap32(0xC11237FF),
    bswap32(0xB21341FF),
    bswap32(0xA3134BFF),
    bswap32(0x931455FF),
    bswap32(0x84145EFF),
    bswap32(0x751568FF),
    bswap32(0x651672FF),
    bswap32(0x56167CFF),
    bswap32(0x471786FF),
    bswap32(0x371790FF),
    bswap32(0x28189AFF),
    bswap32(0x1919A4FF)
    
};
