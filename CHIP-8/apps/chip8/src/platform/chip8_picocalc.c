//----------------------------------------------------
#include "chip8_picocalc.h"
#include "drivers/gfx.h"
#include "drivers/console.h"
#include "drivers/font.h"
#include "pico/stdlib.h"
#include "pico/time.h"   // time_us_64()
#include "keyboard.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


// Pantalla virtual del CHIP-8
#define LCD_W     320
#define LCD_H     240
#define SCALE     5
#define OFFSET_Y  40   // (240 - (32*5)) / 2 = (240 - 160)/2 = 40
#define COLOR_NEGRO  0x0000
#define COLOR_BLANCO 0xFFFF

// "Memoria" de video de los dos frames
static uint64_t vram_prev[CHIP8_VRAM_H];
static bool vram_prev_valid = false;

// Teclas presionadas
static uint8_t key_down[256];

// Barra de titulo
static char g_rom_title[64] = "CHIP-8";
static bool g_title_dirty = true;



// --------------------------------------------------
// RAW PicoCalc -> CHIP-8 key (0..F)
// Layout CHIP-8:
// 1 2 3 C
// 4 5 6 D
// 7 8 9 E
// A 0 B F
// Extras: flechas, ENTER, SPACE
// --------------------------------------------------
typedef struct {
    uint8_t raw;
    uint8_t chip8_key;
} key_map_t;

static const key_map_t keymap[] = {
    // --- Numeros 0..9 ---
    { 0x30, 0x0 }, // '0'
    { 0x31, 0x1 }, // '1'
    { 0x32, 0x2 }, // '2'
    { 0x33, 0x3 }, // '3'
    { 0x34, 0x4 }, // '4'
    { 0x35, 0x5 }, // '5'
    { 0x36, 0x6 }, // '6'
    { 0x37, 0x7 }, // '7'
    { 0x38, 0x8 }, // '8'
    { 0x39, 0x9 }, // '9'

    // --- Letras A..F (mayuscula y minuscula) ---
    { 0x61, 0xA }, { 0x41, 0xA }, // A
    { 0x62, 0xB }, { 0x42, 0xB }, // B
    { 0x63, 0xC }, { 0x43, 0xC }, // C
    { 0x64, 0xD }, { 0x44, 0xD }, // D
    { 0x65, 0xE }, { 0x45, 0xE }, // E
    { 0x66, 0xF }, { 0x46, 0xF }, // F

    // --- Flechas (mapeadas a d-pad clasico) ---
    { 0xB4, 0x4 }, // LEFT  -> 4
    { 0xB7, 0x6 }, // RIGHT -> 6
    { 0xB5, 0x2 }, // UP    -> 2
    { 0xB6, 0x8 }, // DOWN  -> 8

    // --- Extras utiles ---
    { 0x0D, 0x5 }, // ENTER -> 5
    { 0x20, 0x0 }, // SPACE -> 0
};


//---------------------------------------------
static int buscar_chip8_key_por_raw(uint8_t raw)
{
    for (unsigned i = 0; i < (unsigned)(sizeof(keymap) / sizeof(keymap[0])); i++) {
        if (keymap[i].raw == raw) return (int)keymap[i].chip8_key;
    }
    return -1;
}


//---------------------------------------------
//Tecla ESC en menues
static volatile bool g_exit_requested = false;

bool chip8_picocalc_exit_requested(void)
{
    bool v = g_exit_requested;
    g_exit_requested = false;
    return v;
}

//---------------------------------------------
void chip8_picocalc_init(void)
{
    con_clear();  // limpia texto inicial
    gfx_begin();
    keyboard_init();
    keyboard_set_background_poll(true);

    // Reiniciar renderer dirty
    memset(vram_prev, 0, sizeof(vram_prev));
    vram_prev_valid = true;

    g_title_dirty = true;
}

//---------------------------------------------
uint64_t chip8_picocalc_time_us(void)
{
    return time_us_64();
}


//---------------------------------------------
static void clear_screen(void)
{
    gfx_set_color(gfx_rgb(0, 0, 0));
    gfx_fill_rect(0, 0, LCD_W, LCD_H);
}


//---------------------------------------------
void chip8_picocalc_set_title(const char *title)
{
    int i = 0;
    while (title && title[i] && i < (int)sizeof(g_rom_title) - 1) {
        g_rom_title[i] = title[i];
        i++;
    }
    g_rom_title[i] = '\0';
    g_title_dirty = true;
}



//----------------------------------------------
void chip8_picocalc_present(const chip8_t *c8)
{
    // --- Config ---
    const int SCREEN_W  = 320;
    const int SCREEN_H  = 320;
    const int TOP_BAR_H = 16;  // reservado para mostrar nombre del ROM

    const int GAME_W = CHIP8_VRAM_W * SCALE; // 64*SCALE
    const int GAME_H = CHIP8_VRAM_H * SCALE; // 32*SCALE

    int X0 = (SCREEN_W - GAME_W) / 2;
    int Y0 = TOP_BAR_H + ((SCREEN_H - TOP_BAR_H - GAME_H) / 2);

    if (X0 < 0) X0 = 0;
    if (Y0 < TOP_BAR_H) Y0 = TOP_BAR_H;

    // Inicializar estado previo del renderer (una sola vez)
    if (!vram_prev_valid) {
        memset(vram_prev, 0, sizeof(vram_prev));
        vram_prev_valid = true;
    }

    static bool primer_frame = true;

    // --- Primer frame: limpiar y dibujar TODO lo encendido ---
    if (primer_frame) {
        con_clear();

        // Forzar que la barra se dibuje en el primer frame
        g_title_dirty = true;

        // Dibujar barra (fondo + texto) AHORA (no antes)
        gfx_set_color(gfx_rgb(0,90,0));
        gfx_fill_rect(0, 0, SCREEN_W, TOP_BAR_H);

        gfx_set_color(gfx_rgb(255,255,255));
        gfx_text_px(4, 3, g_rom_title, &font_5x10, 1, 2);

        g_title_dirty = false;

        // Bordes  arriba/abajo del area de juego
        gfx_set_color(gfx_rgb(255, 255, 255));
        if (Y0 > 0) gfx_fill_rect(X0, Y0 - 1, GAME_W, 1);
        gfx_fill_rect(X0, Y0 + GAME_H, GAME_W, 1);

        // Dibujar frame completo del juego (blanco)
        gfx_set_color(COLOR_BLANCO);

        for (int y = 0; y < CHIP8_VRAM_H; y++) {
            uint64_t cur = c8->vram[y];
            vram_prev[y] = cur;

            if (cur == 0) continue;

            int x = 0;
            while (x < CHIP8_VRAM_W) {
                while (x < CHIP8_VRAM_W &&
                       (cur & ((uint64_t)1 << (63 - x))) == 0) {
                    x++;
                }
                if (x >= CHIP8_VRAM_W) break;

                int xs = x;
                while (x < CHIP8_VRAM_W &&
                       (cur & ((uint64_t)1 << (63 - x))) != 0) {
                    x++;
                }

                gfx_fill_rect(
                    X0 + xs * SCALE,
                    Y0 + y  * SCALE,
                    (x - xs) * SCALE,
                    SCALE
                );
            }
        }

        primer_frame = false;
        return;
    }

    // --- Barra superior (solo cuando cambia) ---
    if (g_title_dirty) {
        gfx_set_color(gfx_rgb(0,0,0));
        gfx_fill_rect(0, 0, SCREEN_W, TOP_BAR_H);

        gfx_set_color(gfx_rgb(255,255,255));
        gfx_text_px(4, 3, g_rom_title, &font_8x10, 1, 1);

        g_title_dirty = false;
    }

    // --- Frames siguientes: render diferencial (off/on) ---
    for (int y = 0; y < CHIP8_VRAM_H; y++) {

        uint64_t cur  = c8->vram[y];
        uint64_t prev = vram_prev[y];

        if (cur == prev) continue;

        uint64_t off = prev & ~cur;
        uint64_t on  = cur  & ~prev;

        int x;

        // Apagar pixel (NEGRO)
        if (off) {
            gfx_set_color(COLOR_NEGRO);
            x = 0;
            while (x < CHIP8_VRAM_W) {
                while (x < CHIP8_VRAM_W &&
                       (off & ((uint64_t)1 << (63 - x))) == 0) {
                    x++;
                }
                if (x >= CHIP8_VRAM_W) break;

                int xs = x;
                while (x < CHIP8_VRAM_W &&
                       (off & ((uint64_t)1 << (63 - x))) != 0) {
                    x++;
                }

                gfx_fill_rect(
                    X0 + xs * SCALE,
                    Y0 + y  * SCALE,
                    (x - xs) * SCALE,
                    SCALE
                );
            }
        }

        // Encender pixel (BLANCO)
        if (on) {
            gfx_set_color(COLOR_BLANCO);
            x = 0;
            while (x < CHIP8_VRAM_W) {
                while (x < CHIP8_VRAM_W &&
                       (on & ((uint64_t)1 << (63 - x))) == 0) {
                    x++;
                }
                if (x >= CHIP8_VRAM_W) break;

                int xs = x;
                while (x < CHIP8_VRAM_W &&
                       (on & ((uint64_t)1 << (63 - x))) != 0) {
                    x++;
                }

                gfx_fill_rect(
                    X0 + xs * SCALE,
                    Y0 + y  * SCALE,
                    (x - xs) * SCALE,
                    SCALE
                );
            }
        }

        vram_prev[y] = cur;
    }
}



//---------------------------------------------
typedef struct {
    bool pressed;
    uint64_t until_us;
} key_latch_t;

static key_latch_t latch[16];

// cuánto tiempo mantenemos el estos "pressed"
// luego de un evento (en microsegundos)
#define KEY_LATCH_US (120000) // 120 ms


//---------------------------------------------
static void latch_key(int k)
{
    uint64_t now = time_us_64();
    latch[k].pressed = true;
    latch[k].until_us = now + KEY_LATCH_US;
}


//---------------------------------------------
void chip8_picocalc_update_keys(chip8_t *c8)
{
    // Tiempo que se mantiene "apretada" la tecla (para eventos sin key-up real)
    const uint64_t HOLD_US = 200000;

    // "hasta cuando" cada tecla 0..F se considera presionada
    static uint64_t pressed_until[16] = {0};
    uint64_t ahora = time_us_64();

    // Consumir eventos del driver y extender pressed_until
    while (keyboard_key_available()) {
        uint8_t raw = (uint8_t)keyboard_get_key();

        // ESC -> salir al menu
        if (raw == 0xB1) {
            g_exit_requested = true;
            continue;
        }

        int k = buscar_chip8_key_por_raw(raw);
        if (k >= 0 && k < 16) {
            pressed_until[k] = ahora + HOLD_US;
        }
    }

    // Volcar a c8->keys[] como estado estable
    for (int i = 0; i < 16; i++) {
        c8->keys[i] = (ahora < pressed_until[i]) ? 1 : 0;
    }
}