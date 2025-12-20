#include "chip8.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>


//----------------------------------------------------
static void load_font(chip8_t *c8)
{
    // Fuente CHIP-8 clasica (0..F), 5 bytes por caracter, en 0x50
    static const uint8_t font[16 * 5] = {
        0xF0,0x90,0x90,0x90,0xF0, // 0
        0x20,0x60,0x20,0x20,0x70, // 1
        0xF0,0x10,0xF0,0x80,0xF0, // 2
        0xF0,0x10,0xF0,0x10,0xF0, // 3
        0x90,0x90,0xF0,0x10,0x10, // 4
        0xF0,0x80,0xF0,0x10,0xF0, // 5
        0xF0,0x80,0xF0,0x90,0xF0, // 6
        0xF0,0x10,0x20,0x40,0x40, // 7
        0xF0,0x90,0xF0,0x90,0xF0, // 8
        0xF0,0x90,0xF0,0x10,0xF0, // 9
        0xF0,0x90,0xF0,0x90,0x90, // A
        0xE0,0x90,0xE0,0x90,0xE0, // B
        0xF0,0x80,0x80,0x80,0xF0, // C
        0xE0,0x90,0x90,0x90,0xE0, // D
        0xF0,0x80,0xF0,0x80,0xF0, // E
        0xF0,0x80,0xF0,0x80,0x80  // F
    };

    memcpy(&c8->mem[CHIP8_FONT_BASE], font, sizeof(font));
}

//----------------------------------------------------
static inline uint8_t chip8_rand8(chip8_t *c8)
{
    // xorshift32
    uint32_t x = c8->rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    c8->rng_state = x;
    return (uint8_t)(x & 0xFF);
}



//----------------------------------------------------
void chip8_reset(chip8_t *c8)
{
    // Mantener cfg, limpiar resto
    chip8_config_t cfg = c8->cfg;
    memset(c8, 0, sizeof(*c8));
    c8->cfg = cfg;

    c8->pc = CHIP8_ROM_BASE;
    c8->last_key_pressed = -1;
    load_font(c8);

    // Limpiar VRAM y hacer redraw inicial
    for (int y = 0; y < CHIP8_VRAM_H; y++) c8->vram[y] = 0;
    c8->draw_flag = true;
}


//----------------------------------------------------
void chip8_init(chip8_t *c8, chip8_config_t cfg)
{
    // memset(c8, 0, sizeof(*c8));
    // c8->cfg = cfg;
    chip8_reset(c8);
    c8->rng_state = 0x12345678;  // semilla fija (o time_us_32())

    #if CHIP8_DEBUG
        c8->dbg.trace_head = 0;
        c8->dbg.unimpl_count = 0;
        c8->dbg.break_on_unimpl = true;   // Stop on missing opcode
    #endif

}


//----------------------------------------------------
bool chip8_load_rom(chip8_t *c8, const uint8_t *data, uint32_t size)
{
    if (!data || size == 0) return false;
    if (CHIP8_ROM_BASE + size > CHIP8_MEM_SIZE) return false;

    memcpy(&c8->mem[CHIP8_ROM_BASE], data, size);
    c8->pc = CHIP8_ROM_BASE;
    return true;
}


//----------------------------------------------------
void chip8_tick_60hz(chip8_t *c8)
{
    if (c8->delay_timer > 0) c8->delay_timer--;
    if (c8->sound_timer > 0) c8->sound_timer--;
}


//--------------------------------------------------------------
static inline uint16_t fetch_opcode(chip8_t *c8)
{
    uint16_t hi = c8->mem[c8->pc];
    uint16_t lo = c8->mem[c8->pc + 1];
    return (hi << 8) | lo;
}


//----------------------------------------------------
static void op_00E0_cls(chip8_t *c8)
{
    for (int y = 0; y < CHIP8_VRAM_H; y++) {
        c8->vram[y] = 0;
    }
    c8->draw_flag = true;
}


//----------------------------------------------------
static void op_DXYN_draw(chip8_t *c8, uint8_t x, uint8_t y, uint8_t n)
{
    // Coordenadas base (CHIP-8)
    uint8_t x0 = c8->V[x] % CHIP8_VRAM_W;
    uint8_t y0 = c8->V[y] % CHIP8_VRAM_H;

    c8->V[0xF] = 0;

    for (uint8_t row = 0; row < n; row++) {
        uint8_t yy = y0 + row;
        if (c8->cfg.dxyn_wrap) yy %= CHIP8_VRAM_H;
        else if (yy >= CHIP8_VRAM_H) break;

        uint8_t sprite = c8->mem[c8->I + row];

        for (uint8_t bit = 0; bit < 8; bit++) {
            if ((sprite & (0x80 >> bit)) == 0) continue;

            uint8_t xx = x0 + bit;
            if (c8->cfg.dxyn_wrap) xx %= CHIP8_VRAM_W;
            else if (xx >= CHIP8_VRAM_W) break;

            uint64_t mask = (uint64_t)1 << (63 - xx); // bit 63 = x=0
            bool was_on = (c8->vram[yy] & mask) != 0;

            // XOR toggle
            c8->vram[yy] ^= mask;

            // Colisión: si estaba encendido y se apaga
            if (was_on) c8->V[0xF] = 1;
        }
    }

    c8->draw_flag = true;
}


//----------------------------------------------------
#if CHIP8_DEBUG
static inline void chip8_dbg_trace(chip8_t *c8, uint16_t pc, uint16_t op, uint8_t x, uint8_t y)
{
    uint32_t i = c8->dbg.trace_head++ & 0xFF;
    c8->dbg.trace[i].pc = pc;
    c8->dbg.trace[i].op = op;
    c8->dbg.trace[i].I  = c8->I;
    c8->dbg.trace[i].sp = c8->sp;
    c8->dbg.trace[i].dt = c8->delay_timer;
    c8->dbg.trace[i].st = c8->sound_timer;
    c8->dbg.trace[i].vx = c8->V[x & 0xF];
    c8->dbg.trace[i].vy = c8->V[y & 0xF];
}

static void chip8_dbg_dump_last(chip8_t *c8, int n)
{
    if (n > 256) n = 256;
    printf("---- CHIP8 TRACE LAST %d ----\n", n);
    for (int k = n - 1; k >= 0; --k) {
        uint32_t idx = (c8->dbg.trace_head - 1 - (uint32_t)k) & 0xFF;
        printf("%03X: %04X  I=%03X SP=%u DT=%u ST=%u VX=%u VY=%u\n",
               c8->dbg.trace[idx].pc,
               c8->dbg.trace[idx].op,
               c8->dbg.trace[idx].I,
               c8->dbg.trace[idx].sp,
               c8->dbg.trace[idx].dt,
               c8->dbg.trace[idx].st,
               c8->dbg.trace[idx].vx,
               c8->dbg.trace[idx].vy);
    }
}

static inline void chip8_dbg_unimpl(chip8_t *c8, uint16_t pc, uint16_t op)
{
    c8->dbg.unimpl_count++;
    printf("UNIMPL PC=%03X OP=%04X I=%03X SP=%u DT=%u ST=%u (count=%lu)\n",
           pc, op, c8->I, c8->sp, c8->delay_timer, c8->sound_timer,
           (unsigned long)c8->dbg.unimpl_count);

    if (c8->dbg.break_on_unimpl) {
        chip8_dbg_dump_last(c8, 32);
        while (1) { /* freeze para depurar */ }
    }
}
#endif


//----------------------------------------------------
static uint16_t last_unhandled = 0xFFFF;
void chip8_step(chip8_t *c8)
{
    uint16_t pc_before = c8->pc;
    uint16_t op = fetch_opcode(c8);
    c8->pc += 2;

    uint16_t nnn = op & 0x0FFF;
    uint8_t  x   = (op >> 8) & 0x0F;
    uint8_t  y   = (op >> 4) & 0x0F;
    uint8_t  n   = op & 0x0F;

    #if CHIP8_DEBUG
        chip8_dbg_trace(c8, pc_before, op, x, y);
    #endif

    switch (op & 0xF000) {
        case 0x0000:
            if (op == 0x00E0) {
                // CLS
                op_00E0_cls(c8);
            } 
            else if (op == 0x00EE) {
                if (c8->sp == 0) {
                    // stack underflow: ignore or halt (debug)
                    // printf("STACK UNDERFLOW on RET\n");
                } else {
                    c8->sp--;
                    c8->pc = c8->stack[c8->sp];
                }
            }

            else {
                // 0NNN ignored
            }
            break;

        //--------------------------------------------
        case 0x2000:
            if (c8->sp < 16) {
                c8->stack[c8->sp] = c8->pc;
                c8->sp++;
                c8->pc = nnn;
            } else {
                // stack overflow: ignore or halt (debug)
                // printf("STACK OVERFLOW on CALL\n");
            }
            break;

        //--------------------------------------------
        case 0xA000:
            // ANNN: I = NNN (necesario para DXYN)
            c8->I = nnn;
            break;

        //--------------------------------------------
        case 0x6000:
            // 6XNN: Vx = NN (util para test/manual)
            c8->V[x] = (uint8_t)(op & 0x00FF);
            break;

        //--------------------------------------------
        case 0xD000:
            // DXYN: draw
            op_DXYN_draw(c8, x, y, n);
            break;

            
        //--------------------------------------------            
        case 0x1000:
            // 1NNN: jump
            c8->pc = nnn;
            break;


        //--------------------------------------------   
        case 0x7000:
            // 7XNN: Vx += NN (sin carry)
            c8->V[x] = (uint8_t)(c8->V[x] + (op & 0x00FF));
            break;


        //--------------------------------------------
        // Teclado        
        case 0xE000: {
            uint8_t key_index = (uint8_t)(c8->V[x] & 0x0F);

            switch (op & 0x00FF) {
                // EX9E: skip si key[Vx] presionada
                case 0x9E: {
                    uint8_t k = c8->V[x] & 0x0F;

                    //printf("EX9E check k=%X pressed=%d (V%X=%02X)\n", k, c8->keys[k], x, c8->V[x]);

                    if (c8->keys[k]) c8->pc += 2;
                } break;

                // EXA1: skip si key[Vx] NO presionada
                case 0xA1: {
                    uint8_t k = c8->V[x] & 0x0F;

                    //printf("EXA1 check k=%X pressed=%d (V%X=%02X)\n", k, c8->keys[k], x, c8->V[x]);

                    if (!c8->keys[k]) c8->pc += 2;
                } break;

                default:
                    break;
            }
            break;
        }


        //--------------------------------------------
        case 0x3000:
            // 3XNN: skip next instruction if Vx == NN
            if (c8->V[x] == (uint8_t)(op & 0x00FF)) {
                c8->pc += 2;
            }
            break;


        //--------------------------------------------
        case 0x4000:
            // 4XNN: skip next instruction if Vx != NN
            if (c8->V[x] != (uint8_t)(op & 0x00FF)) {
                c8->pc += 2;
            }
            break;

        //--------------------------------------------
        case 0x5000:
            // 5XY0: skip next instruction if Vx == Vy
            if ((op & 0x000F) == 0x0) {
                if (c8->V[x] == c8->V[y]) c8->pc += 2;
            } else {
            #if CHIP8_DEBUG
                chip8_dbg_unimpl(c8, pc_before, op);
            #endif
            }
            break;

        //--------------------------------------------
        case 0x8000: {
            switch (op & 0x000F) {

                // 8XY0: Vx = Vy
                case 0x0:
                    c8->V[x] = c8->V[y];
                    break;

                // 8XY1: Vx = Vx OR Vy
                case 0x1:
                    c8->V[x] |= c8->V[y];
                    break;

                // 8XY2: Vx = Vx AND Vy
                case 0x2:
                    c8->V[x] &= c8->V[y];
                    break;

                // 8XY3: Vx = Vx XOR Vy
                case 0x3:
                    c8->V[x] ^= c8->V[y];
                    break;

                // 8XY4: Vx = Vx + Vy, VF = carry
                case 0x4: {
                    uint16_t sum = (uint16_t)c8->V[x] + (uint16_t)c8->V[y];
                    c8->V[0xF] = (sum > 0xFF) ? 1 : 0;
                    c8->V[x] = (uint8_t)(sum & 0xFF);
                } break;

                // 8XY5: Vx = Vx - Vy, VF = NOT borrow
                case 0x5:
                    c8->V[0xF] = (c8->V[x] >= c8->V[y]) ? 1 : 0;
                    c8->V[x] = (uint8_t)(c8->V[x] - c8->V[y]);
                    break;

                // 8XY6: shift right
                case 0x6: {
                    uint8_t src = c8->cfg.shift_uses_vy ? c8->V[y] : c8->V[x];
                    c8->V[0xF] = (src & 0x01);
                    c8->V[x] = (uint8_t)(src >> 1);
                } break;

                // 8XY7: Vx = Vy - Vx, VF = NOT borrow
                case 0x7:
                    c8->V[0xF] = (c8->V[y] >= c8->V[x]) ? 1 : 0;
                    c8->V[x] = (uint8_t)(c8->V[y] - c8->V[x]);
                    break;

                // 8XYE: shift left
                case 0xE: {
                    uint8_t src = c8->cfg.shift_uses_vy ? c8->V[y] : c8->V[x];
                    c8->V[0xF] = (src & 0x80) ? 1 : 0;
                    c8->V[x] = (uint8_t)(src << 1);
                } break;

                default:
        #if CHIP8_DEBUG
                    chip8_dbg_unimpl(c8, pc_before, op);
        #endif
                    break;
            }
        } break;



        //--------------------------------------------
        case 0x9000:
            // 9XY0: skip next instruction if Vx != Vy
            if ((op & 0x000F) == 0x0) {
                if (c8->V[x] != c8->V[y]) c8->pc += 2;
            } else {
            #if CHIP8_DEBUG
                chip8_dbg_unimpl(c8, pc_before, op);
            #endif
            }
            break;


        //--------------------------------------------
        case 0xB000:
            // BNNN: jump to NNN + V0
            c8->pc = (uint16_t)(nnn + c8->V[0]);
            break;



        //--------------------------------------------
        case 0xC000: { // CXNN: Vx = random & NN
            uint8_t kk = (uint8_t)(op & 0x00FF);
            c8->V[x] = (uint8_t)(chip8_rand8(c8) & kk);
        } break;



        //--------------------------------------------
        case 0xF000: {
            switch (op & 0x00FF) {

                // FX07: Vx = delay_timer
                case 0x07:
                    c8->V[x] = c8->delay_timer;
                    break;

                case 0x0A: {
                    // FX0A: wait for a key press, store key index in Vx
                    int pressed = -1;
                    for (int i = 0; i < 16; i++) {
                        if (c8->keys[i]) { pressed = i; break; }
                    }

                    if (pressed < 0) {
                        // repetir la instruccion
                        c8->pc = pc_before;
                    } else {
                        c8->V[x] = (uint8_t)pressed;
                    }
                } break;


                // FX15: delay_timer = Vx
                case 0x15:
                    c8->delay_timer = c8->V[x];
                    break;

                // FX18: sound_timer = Vx
                case 0x18:
                    c8->sound_timer = c8->V[x];
                    break;

                case 0x1E: {
                    uint16_t before = c8->I;
                    uint16_t after  = (uint16_t)(c8->I + c8->V[x]);

                    c8->I = after;

                    if (c8->cfg.addi_sets_vf) {
                        // criterio típico: overflow si se pasa de 0x0FFF
                        c8->V[0xF] = (after > 0x0FFF) ? 1 : 0;
                    }
                } break;

                // FX29: I = direccion del sprite de fuente para el digito Vx (0..F)
                case 0x29: {
                    uint8_t digit = c8->V[x] & 0x0F;
                    c8->I = (uint16_t)(CHIP8_FONT_BASE + (digit * 5));
                } break;

                // FX33: BCD de Vx en mem[I..I+2]
                case 0x33: {
                    uint8_t val = c8->V[x];
                    c8->mem[c8->I + 0] = (uint8_t)(val / 100);
                    c8->mem[c8->I + 1] = (uint8_t)((val / 10) % 10);
                    c8->mem[c8->I + 2] = (uint8_t)(val % 10);
                } break;

                case 0x55: {
                    // FX55: store V0..Vx in mem[I..]
                    for (uint8_t i = 0; i <= x; i++) {
                        c8->mem[c8->I + i] = c8->V[i];
                    }
                    if (c8->cfg.bulk_inc_i) {
                        c8->I = (uint16_t)(c8->I + x + 1);
                    }
                } break;

                case 0x65: {
                    // FX65: load V0..Vx from mem[I..]
                    for (uint8_t i = 0; i <= x; i++) {
                        c8->V[i] = c8->mem[c8->I + i];
                    }
                    if (c8->cfg.bulk_inc_i) {
                        c8->I = (uint16_t)(c8->I + x + 1);
                    }
                } break;


                default:
                    break;
            }
        } break;


        default:
            if (op != last_unhandled) {
                last_unhandled = op;
                #if CHIP8_DEBUG
                    chip8_dbg_unimpl(c8, pc_before, op);
                #endif
            }
            break;

            }

}
