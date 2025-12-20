#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    // Frecuencia de CPU (steps por segundo)
    uint32_t cpu_hz;

    // Quirks (compatibilidad con distintas ROMs)
    // (Mas adelante implementar como "config")
    bool shift_uses_vy;   // 8XY6 / 8XYE usan Vy como fuente
    bool addi_sets_vf;    // FX1E setea VF si hay overflow
    bool bulk_inc_i;      // FX55/FX65 incrementan I al finalizar
    bool dxyn_wrap;       // DXYN hace wrap en bordes (vs clip)
} chip8_config_t;

//----------------------------------------------------
// Config "clasico" razonable para arrancar
static inline chip8_config_t chip8_config_default(void)
{
    chip8_config_t c = {0};
    c.cpu_hz = 500;

    c.shift_uses_vy = false;
    c.addi_sets_vf  = false;
    c.bulk_inc_i    = false;
    c.dxyn_wrap     = true;

    return c;
}
