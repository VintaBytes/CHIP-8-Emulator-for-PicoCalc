#pragma once
#include <stdint.h>
#include "drivers/font.h"

void con_set_font_x(uint8_t n);
void con_set_font_y(uint8_t n);

// --- API base ---
void con_putc(char c);
void con_print(const char *s);

// --- Control de pantalla ---
void con_clear(void);              // limpia pantalla + cursor home
void con_home(void);               // cursor a 1,1
void con_goto(uint8_t row, uint8_t col);  // 1-based (fila, columna)



// Color de texto
typedef enum {
    CON_BLACK   = 1,
    CON_RED     = 2,
    CON_GREEN   = 3,
    CON_YELLOW  = 4,
    CON_BLUE    = 5,
    CON_MAGENTA = 6,
    CON_CYAN    = 7,
    CON_WHITE   = 8,

    CON_BRIGHT_BLACK   = 9,
    CON_BRIGHT_RED     = 10,
    CON_BRIGHT_GREEN   = 11,
    CON_BRIGHT_YELLOW  = 12,
    CON_BRIGHT_BLUE    = 13,
    CON_BRIGHT_MAGENTA = 14,
    CON_BRIGHT_CYAN    = 15,
    CON_BRIGHT_WHITE   = 16
} con_color_t;


// fg: color de frente (obligatorio)
// bg: color de fondo, 0 = no cambiar, -1 = transparente (futuro)
void con_text_color(con_color_t fg, int bg);


void con_reset(void);              // SGR 0
void con_bold(int enabled);        // SGR 1 / 22
void con_underline(int enabled);   // SGR 4 / 24
void con_fg(con_color_t color);
void con_bg(con_color_t color);
void con_set_font(const font_t *font);
void con_save_cursor(void);    // ESC 7  (o ESC[s)
void con_restore_cursor(void); // ESC 8  (o ESC[u)
void con_reverse(int enabled);

// ---------------------------------------------------------
// Entrada de texto
// ---------------------------------------------------------
int con_input(char *buf, int maxlen);
int con_input_fixed(char *buf, int width);
int con_input_int(int *out);

// ---------------------------------------------------------
// Salida con formato
// ---------------------------------------------------------
void con_printf(const char *fmt, ...);