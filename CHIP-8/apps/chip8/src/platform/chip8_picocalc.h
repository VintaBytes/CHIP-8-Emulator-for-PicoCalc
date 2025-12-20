#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../core/chip8.h"
#include "drivers/gfx.h"  

uint64_t chip8_picocalc_time_us(void);

void chip8_picocalc_init(void);
void chip8_picocalc_present(const chip8_t *c8);
void chip8_picocalc_update_keys(chip8_t *c8);
void chip8_picocalc_set_title(const char *title);
bool chip8_picocalc_exit_requested(void); //Tecla Escape