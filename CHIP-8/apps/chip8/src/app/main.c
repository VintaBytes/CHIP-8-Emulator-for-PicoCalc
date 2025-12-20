//----------------------------------------------------
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/time.h"
#include "drivers/picocalc.h"
#include "drivers/fat32.h"
#include "drivers/console.h"
#include "keyboard.h"
#include "drivers/gfx.h"
#include "rom_menu.h"

#include "../core/chip8.h"
#include "../platform/chip8_picocalc.h"


//----------------------------------------------------
void user_interrupt(void) { }

//----------------------------------------------------
// Lee un archivo completo desde FAT32 (ya abierto) a un buffer.
// Devuelve FAT32_OK si pudo leer (aunque sea 1 byte) sin error.

static fat32_error_t leer_fat32_completo(fat32_file_t *f, uint8_t *dst, uint32_t max_len, uint32_t *out_len)
{
    fat32_error_t err = FAT32_OK;
    size_t br = 0;
    *out_len = 0;

    while (*out_len < max_len) {
        uint32_t space = max_len - *out_len;
        err = fat32_read(f, dst + *out_len, space, &br);
        if (err != FAT32_OK) return err;
        if (br == 0) break;
        *out_len += (uint32_t)br;
    }
    return FAT32_OK;
}


//----------------------------------------------------
static bool cargar_rom_a_chip8(chip8_t *c8, const char *rom_path)
{
    fat32_file_t f;
    fat32_error_t err = fat32_open(&f, rom_path);
    if (err != FAT32_OK) return false;

    static uint8_t rom_buf[CHIP8_MEM_SIZE - CHIP8_ROM_BASE];
    uint32_t rom_len = 0;

    err = leer_fat32_completo(&f, rom_buf, sizeof(rom_buf), &rom_len);
    fat32_close(&f);

    if (err != FAT32_OK || rom_len == 0) return false;
    return chip8_load_rom(c8, rom_buf, rom_len);
}


//----------------------------------------------------
static void run_rom(const char *rom_path)
{
    chip8_t c8;
    chip8_init(&c8, chip8_config_default());

    // titulo (ruta completa)
    chip8_picocalc_set_title(rom_path);

    // fuerza 1er frame
    bool frame_dirty = true;
    c8.draw_flag = true;

    if (!cargar_rom_a_chip8(&c8, rom_path)) {
        con_clear();
        con_print("No se pudo cargar ROM:\n");
        con_print(rom_path);
        con_print("\n\nTecla para volver...\n");
        while (!keyboard_key_available()) { }
        (void)keyboard_get_key();
        return;
    }


    //----------------------------------------------------
    const uint32_t CPU_HZ = 550;
    const uint32_t TIMER_HZ = 60;

    const uint64_t CPU_STEP_US   = 1000000ULL / CPU_HZ;
    const uint64_t TIMER_STEP_US = 1000000ULL / TIMER_HZ;
    const uint64_t FRAME_STEP_US = 1000000ULL / 60;

    uint64_t t_prev_us = time_us_64();
    uint64_t acc_cpu_us = 0, acc_timer_us = 0, acc_frame_us = 0;

    while (true) {
        uint64_t t_now_us = time_us_64();
        uint64_t dt_us = t_now_us - t_prev_us;
        t_prev_us = t_now_us;
        if (dt_us > 100000) dt_us = 100000;

        acc_cpu_us   += dt_us;
        acc_timer_us += dt_us;
        acc_frame_us += dt_us;

        chip8_picocalc_update_keys(&c8);

        // ESC vuelve al menu
        if (chip8_picocalc_exit_requested()) {
            return;
        }

        int steps = 0;
        while (acc_cpu_us >= CPU_STEP_US && steps < 50) {
            chip8_step(&c8);
            acc_cpu_us -= CPU_STEP_US;
            steps++;

            if (c8.draw_flag) {
                frame_dirty = true;
                c8.draw_flag = false;
            }
        }

        while (acc_timer_us >= TIMER_STEP_US) {
            chip8_tick_60hz(&c8);
            acc_timer_us -= TIMER_STEP_US;
        }

        while (acc_frame_us >= FRAME_STEP_US) {
            if (frame_dirty) {
                chip8_picocalc_present(&c8);
                frame_dirty = false;
            }
            acc_frame_us -= FRAME_STEP_US;
        }

        tight_loop_contents();
    }
}


//----------------------------------------------------
// Bucle principal
//----------------------------------------------------
int main(void)
{
    stdio_init_all();
    picocalc_init();

    // Preparar consola para el menu
    con_reset();
    con_clear();

    char rom_path[64];

    while (rom_menu_select(rom_path, sizeof(rom_path))) {
        // Antes de correr la ROM, pasamos a modo "juego"
        chip8_picocalc_init();
        run_rom(rom_path);

        // Al volver (ESC), restaurar consola para el menu
        con_reset();
        con_clear();
        bool frame_dirty = true;
    }

    con_clear();
    con_print("Saliendo...\n");
    while (true) tight_loop_contents();
}

