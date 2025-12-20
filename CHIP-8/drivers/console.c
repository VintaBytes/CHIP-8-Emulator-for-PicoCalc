// ---------------------------------------------------------
#include "drivers/console.h"
#include "drivers/lcd.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>   // getchar
#include <stdarg.h>
#include <string.h>  // strncpy
#include <stdlib.h>  // strtol


// ---------------------------------------------------------
// Teclas para input()
// ---------------------------------------------------------
#define KEY_LEFT    0xB4
#define KEY_RIGHT   0xB7
#define KEY_UP      0xB5
#define KEY_DOWN    0xB6
#define KEY_HOME    0xD2
#define KEY_END     0xD5
#define KEY_DELETE  0xD4

// ---------------------------------------------------------
// Estado interno de la consola (0-based)
// ---------------------------------------------------------
static uint16_t con_col = 0;
static uint16_t con_row = 0;
static uint16_t con_saved_col = 0;
static uint16_t con_saved_row = 0;
static uint8_t con_tabsize = 4;

// ---------------------------------------------------------
// Util: limites actuales (dependen de font + escala)
// ---------------------------------------------------------
// Devuelve la última columna válida (0-based) 
// según el font y la escala actuales.
static inline uint16_t con_max_col(void) {
    int c = (int)lcd_get_columns() - 1;
    return (c < 0) ? 0 : (uint16_t)c;
}

// Devuelve la última fila válida (0-based) 
// según el font y la escala actuales.
static inline uint16_t con_max_row(void) {
    int r = (int)lcd_get_rows() - 1;
    return (r < 0) ? 0 : (uint16_t)r;
}

// Asegura que el cursor lógico no se salga de la pantalla.
static inline void con_clamp_cursor(void) {
    uint16_t mc = con_max_col();
    uint16_t mr = con_max_row();
    if (con_col > mc) con_col = mc;
    if (con_row > mr) con_row = mr;
}

// Mueve el cursor del LCD a (con_col, con_row) y lo dibuja.
static inline void con_apply_cursor(void) {
    lcd_move_cursor((uint8_t)con_col, (uint8_t)con_row);
    lcd_draw_cursor();
}

// ---------------------------------------------------------
// Paleta minima RGB565 (16 colores)
// LA VOY A DEJAR DE USAR EN EL FUTURO
// ---------------------------------------------------------
static const uint16_t pal8[8] = {
    0x0000, // black
    0xF800, // red
    0x07E0, // green
    0xFFE0, // yellow
    0x001F, // blue
    0xF81F, // magenta
    0x07FF, // cyan
    0xFFFF  // white
};

static const uint16_t pal8_bright[8] = {
    0x7BEF, // bright black (gris)
    0xF800, // bright red
    0x07E0, // bright green
    0xFFE0, // bright yellow
    0x001F, // bright blue
    0xF81F, // bright magenta
    0x07FF, // bright cyan
    0xFFFF  // bright white
};

static uint16_t con_color_to_rgb565(con_color_t color, bool is_bg)
{
    int code = (int)color;

    // Defaults
    if ((!is_bg && code == 39) || (is_bg && code == 49)) {
        return is_bg ? 0x0000 : 0xFFFF;
    }

    // Normal FG 30..37
    if (!is_bg && code >= 30 && code <= 37) {
        return pal8[code - 30];
    }

    // Bright FG 90..97
    if (!is_bg && code >= 90 && code <= 97) {
        return pal8_bright[code - 90];
    }

    // Normal BG 40..47
    if (is_bg && code >= 40 && code <= 47) {
        return pal8[code - 40];
    }

    // Bright BG 100..107
    if (is_bg && code >= 100 && code <= 107) {
        return pal8_bright[code - 100];
    }

    // Si llega algo raro, fallback
    return is_bg ? 0x0000 : 0xFFFF;
}

// ---------------------------------------------------------
// API fuente / escala
// ---------------------------------------------------------
void con_set_font(const font_t *font) {
    lcd_erase_cursor();
    lcd_set_font(font);
    con_clamp_cursor();
    con_apply_cursor();
}

void con_set_font_x(uint8_t n) {
    lcd_erase_cursor();
    lcd_set_font_scale_x(n);
    con_clamp_cursor();
    con_apply_cursor();
}

void con_set_font_y(uint8_t n) {
    lcd_erase_cursor();
    lcd_set_font_scale_y(n);
    con_clamp_cursor();
    con_apply_cursor();
}

// ---------------------------------------------------------
// ESTABLECER COLOR FRENTE/FONDO DEL TEXTO
// ---------------------------------------------------------
static const uint16_t con_palette[] = {
    0x0000, // dummy (index 0 no se usa)

    0x0000, // BLACK
    0xF800, // RED
    0x07E0, // GREEN
    0xFFE0, // YELLOW
    0x001F, // BLUE
    0xF81F, // MAGENTA
    0x07FF, // CYAN
    0xFFFF, // WHITE

    0x7BEF, // BRIGHT BLACK (gris)
    0xF800, // BRIGHT RED
    0x07E0, // BRIGHT GREEN
    0xFFE0, // BRIGHT YELLOW
    0x001F, // BRIGHT BLUE
    0xF81F, // BRIGHT MAGENTA
    0x07FF, // BRIGHT CYAN
    0xFFFF  // BRIGHT WHITE
};


void con_text_color(con_color_t fg, int bg)
{
    // --- Foreground (siempre) ---
    if (fg > 0 && fg < (int)(sizeof(con_palette)/sizeof(con_palette[0]))) {
        lcd_set_foreground(con_palette[fg]);
    }

    // --- Background ---
    if (bg == 0) {
        return; // no cambiar fondo
    }

    if (bg == -1) {
        // transparente (en el futuro)
        return;
    }

    if (bg > 0 && bg < (int)(sizeof(con_palette)/sizeof(con_palette[0]))) {
        lcd_set_background(con_palette[bg]);
    }
}




// ---------------------------------------------------------
// Control de pantalla
// ---------------------------------------------------------
void con_clear(void) {
    lcd_clear_screen();
    con_col = 0;
    con_row = 0;
    lcd_cursor_reset(); 
    con_apply_cursor();   
}

void con_home(void) {
    con_col = 0;
    con_row = 0;
    lcd_cursor_reset(); 
    con_apply_cursor();
}

void con_goto(uint8_t row, uint8_t col) {
    lcd_erase_cursor();

    // API 1-based
    con_row = (row > 0) ? (uint16_t)(row - 1) : 0;
    con_col = (col > 0) ? (uint16_t)(col - 1) : 0;

    con_clamp_cursor();
    con_apply_cursor();
}

void con_save_cursor(void) {
    con_saved_col = con_col;
    con_saved_row = con_row;
}

void con_restore_cursor(void) {
    lcd_erase_cursor();
    con_col = con_saved_col;
    con_row = con_saved_row;
    con_clamp_cursor();
    con_apply_cursor();
}

// ---------------------------------------------------------
// Estilo (sin ANSI)
// ---------------------------------------------------------
void con_reset(void) {
    // defaults 
    lcd_set_foreground(0xFFFF);
    lcd_set_background(0x0000);
    lcd_set_bold(false);
    lcd_set_underscore(false);
    lcd_set_reverse(false);
}

void con_bold(int enabled) {
    lcd_set_bold(enabled != 0);
}

void con_underline(int enabled) {
    lcd_set_underscore(enabled != 0);
}

void con_reverse(int enabled) {
    lcd_set_reverse(enabled != 0);
}

void con_fg(con_color_t color) {
    uint16_t rgb = con_color_to_rgb565(color, false);
    lcd_set_foreground(rgb);
}

void con_bg(con_color_t color) {
    uint16_t rgb = con_color_to_rgb565(color, true);
    lcd_set_background(rgb);
}

// ---------------------------------------------------------
// Emision "simple" (sin ANSI)
// TAB=4, BS solo mueve cursor
void con_putc(char c)
{
    uint16_t mc = con_max_col();
    uint16_t mr = con_max_row();

    lcd_erase_cursor();

    switch ((unsigned char)c)
    {
        case '\r':  // CR
            con_col = 0;
            break;

        case '\n':  // LF
            con_row++;
            con_col = 0;
            break;

        case '\b':  // BS (solo mover)
            if (con_col > 0) con_col--;
            break;

        case '\t':  // TAB
        {
            uint16_t next = (uint16_t)(((con_col / con_tabsize) + 1) * con_tabsize);
            con_col = next;
            break;
        }

        default:
            if ((unsigned char)c >= 0x20 && (unsigned char)c < 0x7F) {
                lcd_putc((uint8_t)con_col, (uint8_t)con_row, (uint8_t)c);
                con_col++;
            }
            break;
    }

    // Wrap
    if (con_col > mc) {
        con_col = 0;
        con_row++;
    }

    // Scroll
    while (con_row > mr) {
        lcd_scroll_up();
        con_row--;
    }

    con_apply_cursor();
}


// ---------------------------------------------------------
void con_print(const char *s) {
    while (*s) con_putc(*s++);
}

// ---------------------------------------------------------
void con_printf(const char *fmt, ...)
{
    char buf[128];   // tamaño razonable para UI
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    con_print(buf);
}


// ---------------------------------------------------------
// Input
// ---------------------------------------------------------
int con_input(char *buf, int maxlen)
{
    int len = 0;
    int cur = 0;
    char c;

    if (!buf || maxlen <= 1)
        return 0;

    buf[0] = '\0';

    for (;;)
    {
        c = getchar();

        /* ENTER */
        if (c == '\n' || c == '\r')
        {
            con_putc('\n');
            buf[len] = '\0';
            return len;
        }

        /* HOME */
        if (c == KEY_HOME)
        {
            while (cur > 0)
            {
                con_putc('\b');
                cur--;
            }
            continue;
        }

        /* END */
        if (c == KEY_END)
        {
            while (cur < len)
            {
                con_putc(buf[cur]);
                cur++;
            }
            continue;
        }

        /* LEFT */
        if (c == KEY_LEFT)
        {
            if (cur > 0)
            {
                con_putc('\b');
                cur--;
            }
            continue;
        }

        /* RIGHT */
        if (c == KEY_RIGHT)
        {
            if (cur < len)
            {
                con_putc(buf[cur]);
                cur++;
            }
            continue;
        }

        /* BACKSPACE */
        if (c == '\b')
        {
            if (cur > 0)
            {
                /* mover cursor */
                con_putc('\b');

                /* mover buffer */
                for (int i = cur - 1; i < len - 1; i++)
                    buf[i] = buf[i + 1];

                len--;
                cur--;

                /* redibujar resto */
                for (int i = cur; i < len; i++)
                    con_putc(buf[i]);

                /* borrar último */
                con_putc(' ');
                con_putc('\b');

                /* volver cursor */
                for (int i = cur; i < len; i++)
                    con_putc('\b');

                buf[len] = '\0';
            }
            continue;
        }

        /* DELETE */
        if (c == KEY_DELETE)
        {
            if (cur < len)
            {
                for (int i = cur; i < len - 1; i++)
                    buf[i] = buf[i + 1];

                len--;

                for (int i = cur; i < len; i++)
                    con_putc(buf[i]);

                con_putc(' ');
                con_putc('\b');

                for (int i = cur; i < len; i++)
                    con_putc('\b');

                buf[len] = '\0';
            }
            continue;
        }

        /* INSERT */
        if (c >= 0x20 && c < 0x7F)
        {
            if (len < maxlen - 1)
            {
                for (int i = len; i > cur; i--)
                    buf[i] = buf[i - 1];

                buf[cur] = c;
                len++;

                for (int i = cur; i < len; i++)
                    con_putc(buf[i]);

                cur++;

                for (int i = cur; i < len; i++)
                    con_putc('\b');

                buf[len] = '\0';
            }
        }
    }
}



// ---------------------------------------------------------
int con_input_fixed(char *buf, int width)
{
    char tmp[128];

    int n = con_input(tmp, width + 1);
    strncpy(buf, tmp, width);
    buf[width] = '\0';

    return n;
}

// ---------------------------------------------------------
int con_input_int(int *out)
{
    char buf[32];

    while (1)
    {
        con_input(buf, sizeof(buf));

        char *end;
        long v = strtol(buf, &end, 10);

        if (end != buf && *end == '\0')
        {
            *out = (int)v;
            return 1;
        }

        con_print("Numero invalido. Reintente:\n");
    }
}


