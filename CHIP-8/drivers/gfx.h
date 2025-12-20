#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <drivers/font.h>


// -------------------------------------------------
// Inicialización
// -------------------------------------------------
void gfx_begin(void);
void gfx_end(void);

// -------------------------------------------------
// Color
// -------------------------------------------------
void gfx_set_color(uint16_t rgb);
uint16_t gfx_get_color(void);

// Helper RGB888 -> RGB565
uint16_t gfx_rgb(uint8_t r, uint8_t g, uint8_t b);

// -------------------------------------------------
// Primitivas
// -------------------------------------------------
void gfx_pixel(int x, int y);
void gfx_line(int x0, int y0, int x1, int y1);

void gfx_rect(int x, int y, int w, int h);
void gfx_fill_rect(int x, int y, int w, int h);

void gfx_circle(int cx, int cy, int r);      
void gfx_fill_circle(int cx, int cy, int r); 

void gfx_text_px(
    int x,
    int y,
    const char *text,
    const font_t *font,
    uint8_t scale_x,
    uint8_t scale_y
);


bool gfx_draw_bmp(const char *path, int x, int y);
