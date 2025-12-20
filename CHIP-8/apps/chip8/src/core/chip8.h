#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "chip8_config.h"
#include "keyboard.h"

// --- Debug/tracing (activar/desactivar) ---
#ifndef CHIP8_DEBUG
#define CHIP8_DEBUG 0
#endif

#define CHIP8_MEM_SIZE   4096
#define CHIP8_STACK_SIZE 16
#define CHIP8_ROM_BASE   0x200
#define CHIP8_FONT_BASE  0x50

#define CHIP8_VRAM_W 64
#define CHIP8_VRAM_H 32


//----------------------------------------------------
typedef struct {
    chip8_config_t cfg;

    // CPU
    uint16_t pc;
    uint16_t I;
    uint8_t  V[16];

    // Memoria
    uint8_t mem[CHIP8_MEM_SIZE];

    // Stack
    uint16_t stack[CHIP8_STACK_SIZE];
    uint8_t  sp;

    // Timers (decrementan a 60 Hz)
    uint8_t delay_timer;
    uint8_t sound_timer;

    // Input CHIP-8
    uint8_t keys[16];          // 1 = presionada
    int8_t  last_key_pressed;  // para FX0A, -1 si ninguna

    // Video 64x32 (1 bit por pixel)
    uint64_t vram[CHIP8_VRAM_H];

    // Flags
    bool draw_flag;            // pedir redraw

    uint32_t rng_state;

    uint8_t fb[32][64];


    #if CHIP8_DEBUG
        struct {
            uint32_t trace_head;

            struct {
                uint16_t pc;
                uint16_t op;
                uint16_t I;
                uint8_t  sp;
                uint8_t  dt, st;
                uint8_t  vx, vy;
            } trace[256];

            uint32_t unimpl_count;
            bool break_on_unimpl;
        } dbg;
    #endif

} chip8_t;

// --- Ciclo de vida ---
void chip8_init(chip8_t *c8, chip8_config_t cfg);
void chip8_reset(chip8_t *c8);

// --- ROM ---
bool chip8_load_rom(chip8_t *c8, const uint8_t *data, uint32_t size);

// --- Ejecución ---
void chip8_step(chip8_t *c8);      // ejecuta 1 opcode
void chip8_tick_60hz(chip8_t *c8); // decrementa DT/ST (si > 0)

// --- Helpers útiles ---
static inline void chip8_set_key(chip8_t *c8, uint8_t key, bool down)
{
    if (key < 16) c8->keys[key] = down ? 1 : 0;
}




