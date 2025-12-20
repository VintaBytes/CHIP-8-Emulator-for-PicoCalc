#include "drivers/gfx.h"
#include "drivers/lcd.h"
#include "drivers/fat32.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


// Estado minimo
static uint16_t g_color = 0xFFFF; // blanco por defecto


//----------------------------------------------------
void gfx_begin(void)
{
    // Por ahora no hace nada, pero queda para futuras mejoras
}

//----------------------------------------------------
void gfx_end(void)
{
    // Por ahora no hace nada
}

//----------------------------------------------------
void gfx_set_color(uint16_t rgb)
{
    g_color = rgb;
}

//----------------------------------------------------
uint16_t gfx_get_color(void)
{
    return g_color;
}

//----------------------------------------------------
uint16_t gfx_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    // RGB888 -> RGB565
    return (uint16_t)(((r & 0xF8) << 8) |
                      ((g & 0xFC) << 3) |
                      ((b & 0xF8) >> 3));
}

//----------------------------------------------------
static inline bool in_bounds(int x, int y)
{
    // WIDTH y HEIGHT vienen de lcd.h (ya existen en tu proyecto)
    return (x >= 0 && x < (int)WIDTH && y >= 0 && y < (int)HEIGHT);
}


// -------------------------------------------------
// Pixel
// -------------------------------------------------
void gfx_pixel(int x, int y)
{
    if (!in_bounds(x, y)) return;

    // Opcion simple: blit de 1 pixel (1x1)
    uint16_t px = g_color;
    lcd_blit(&px, (uint16_t)x, (uint16_t)y, 1, 1);
}


// -------------------------------------------------
// Lineas
// -------------------------------------------------
void gfx_line(int x0, int y0, int x1, int y1)
{
    // Bresenham entero clasico
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0); // negativo
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;)
    {
        gfx_pixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;

        int e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}


// -------------------------------------------------
// Rectangulos
// -------------------------------------------------
void gfx_rect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;

    int x2 = x + w - 1;
    int y2 = y + h - 1;

    gfx_line(x,  y,  x2, y);
    gfx_line(x,  y2, x2, y2);
    gfx_line(x,  y,  x,  y2);
    gfx_line(x2, y,  x2, y2);
}

void gfx_fill_rect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;

    // Relleno por scanlines
    for (int yy = y; yy < y + h; yy++)
    {
        gfx_line(x, yy, x + w - 1, yy);
    }
}

// -------------------------------------------------
// Circulos
// -------------------------------------------------
void gfx_circle(int cx, int cy, int r)
{
    if (r <= 0) return;

    int x = 0;
    int y = r;
    int d = 1 - r;   // decision variable

    while (x <= y)
    {
        // 8 puntos simétricos
        gfx_pixel(cx + x, cy + y);
        gfx_pixel(cx - x, cy + y);
        gfx_pixel(cx + x, cy - y);
        gfx_pixel(cx - x, cy - y);

        gfx_pixel(cx + y, cy + x);
        gfx_pixel(cx - y, cy + x);
        gfx_pixel(cx + y, cy - x);
        gfx_pixel(cx - y, cy - x);

        if (d < 0)
        {
            d += (2 * x) + 3;
        }
        else
        {
            d += (2 * (x - y)) + 5;
            y--;
        }

        x++;
    }
}

// -------------------------------------------------

// Helper para los scanlines
static void gfx_hspan_fast(int x0, int x1, int y)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    gfx_line(x0, y, x1, y);
}

void gfx_fill_circle(int cx, int cy, int r)
{
    if (r <= 0) return;

    int x = 0;
    int y = r;
    int d = 1 - r;

    while (x <= y)
    {
        gfx_hspan_fast(cx - x, cx + x, cy + y);
        gfx_hspan_fast(cx - x, cx + x, cy - y);

        gfx_hspan_fast(cx - y, cx + y, cy + x);
        gfx_hspan_fast(cx - y, cx + y, cy - x);

        if (d < 0) {
            d += (2 * x) + 3;
        } else {
            d += (2 * (x - y)) + 5;
            y--;
        }
        x++;
    }
}


// -------------------------------------------------
// Texto
// -------------------------------------------------
void gfx_text_px(
    int x,
    int y,
    const char *text,
    const font_t *font,
    uint8_t scale_x,
    uint8_t scale_y
)
{
    if (!text || scale_x == 0 || scale_y == 0)
        return;

    int start_x = x;
    int cx = x;
    int cy = y;

    // const font_t *f = lcd_get_font();   
    if (!font) return;

    while (*text)
    {
        char c = *text++;

        if (c == '\n')
        {
            cx = start_x;
            cy += GLYPH_HEIGHT * scale_y;
            continue;
        }

        if ((unsigned char)c < 32 || (unsigned char)c > 126)
        {
            cx += font->width * scale_x;
            continue;
        }

        const uint8_t *glyph = &font->glyphs[(uint8_t)c * GLYPH_HEIGHT];

        for (int gy = 0; gy < GLYPH_HEIGHT; gy++)
        {
            uint8_t row = glyph[gy];

            for (int gx = 0; gx < font->width; gx++)
            {
                if (row & (1 << (font->width - 1 - gx)))
                {
                    // bloque escalado
                    for (int sy = 0; sy < scale_y; sy++)
                        for (int sx = 0; sx < scale_x; sx++)
                            gfx_pixel(cx + gx * scale_x + sx,
                                      cy + gy * scale_y + sy);
                }
            }
        }

        cx += font->width * scale_x;
    }
}






// -------------------------------------------------
// Helpers LE (little-endian)
// -------------------------------------------------
static inline uint16_t rd_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t rd_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}
static inline int32_t rd_i32_le(const uint8_t *p) {
    return (int32_t)rd_u32_le(p);
}

// Color clave transparencia (#FF00FF)
#define KEY_RGB565 0xF81F

// BI_RGB = 0 (sin compresion), BI_BITFIELDS = 3 (masks)
#define BI_RGB      0u
#define BI_BITFIELDS 3u

// Limite razonable para tiles
#define GFX_BMP_MAX_W  128
#define GFX_BMP_MAX_H  128

static inline bool in_bounds_px(int px, int py) {
    return (px >= 0 && px < (int)WIDTH && py >= 0 && py < (int)HEIGHT);
}

// Convierte 24bpp BGR -> RGB565
static inline uint16_t bgr24_to_rgb565(uint8_t b, uint8_t g, uint8_t r) {
    // 5/6/5
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Row stride BMP: filas alineadas a 4 bytes
static inline uint32_t bmp_row_stride(uint32_t w, uint16_t bpp) {
    uint32_t bits = (uint32_t)w * (uint32_t)bpp;
    uint32_t bytes = (bits + 7u) / 8u;
    return (bytes + 3u) & ~3u;
}

// -------------------------------------------------
// Dibuja BMP en (x,y) en pixeles. Usa transparencia fucsia.
// Soporta: 16bpp RGB565 y 24bpp.
// -------------------------------------------------
bool gfx_draw_bmp(const char *path, int x, int y)
{
    fat32_file_t f;
    fat32_error_t err;

    err = fat32_open(&f, path);
    if (err != FAT32_OK) return false;

    // --- Leer header BMP (14 bytes) + DIB header size minimo (40) ---
    uint8_t file_hdr[14];
    size_t br = 0;

    err = fat32_read(&f, file_hdr, sizeof(file_hdr), &br);
    if (err != FAT32_OK || br != sizeof(file_hdr)) { fat32_close(&f); return false; }

    // Signature "BM"
    if (file_hdr[0] != 'B' || file_hdr[1] != 'M') { fat32_close(&f); return false; }

    uint32_t bfOffBits = rd_u32_le(&file_hdr[10]);

    // Leer tamanio del DIB header
    uint8_t dib_size_buf[4];
    err = fat32_read(&f, dib_size_buf, 4, &br);
    if (err != FAT32_OK || br != 4) { fat32_close(&f); return false; }

    uint32_t dibSize = rd_u32_le(dib_size_buf);
    if (dibSize < 40 || dibSize > 124) { //  BITMAPINFOHEADER y variantes comunes
        fat32_close(&f);
        return false;
    }

    // Leer resto del DIB header
    uint8_t dib[124];
    memset(dib, 0, sizeof(dib));

    uint32_t to_read = dibSize - 4;
    err = fat32_read(&f, dib, to_read, &br);
    if (err != FAT32_OK || br != to_read) { fat32_close(&f); return false; }

    // Campos (BITMAPINFOHEADER compatible)
    int32_t  biWidth      = rd_i32_le(&dib[0]);   // offset 4 en el DIB total, pero aca dib[] arranca en offset 4
    int32_t  biHeight     = rd_i32_le(&dib[4]);
    uint16_t biPlanes     = rd_u16_le(&dib[8]);
    uint16_t biBitCount   = rd_u16_le(&dib[10]);
    uint32_t biCompression= rd_u32_le(&dib[12]);

    if (biPlanes != 1) { fat32_close(&f); return false; }
    if (biWidth <= 0) { fat32_close(&f); return false; }

    bool top_down = false;
    if (biHeight < 0) { top_down = true; biHeight = -biHeight; }
    if (biHeight <= 0) { fat32_close(&f); return false; }

    if (biWidth > GFX_BMP_MAX_W || biHeight > GFX_BMP_MAX_H) {
        fat32_close(&f);
        return false;
    }

    if (!(biCompression == BI_RGB || biCompression == BI_BITFIELDS)) {
        fat32_close(&f);
        return false;
    }

    if (!(biBitCount == 16 || biBitCount == 24)) {
        fat32_close(&f);
        return false;
    }

    // Si es 16bpp y BI_BITFIELDS, pueden venir masks; si es BI_RGB asumimos RGB565 (muy comun en embedded)
    // Nota: no parseo masks por ahora (se puede agregar luego si aparece un BMP 555)

    uint32_t stride = bmp_row_stride((uint32_t)biWidth, biBitCount);

    // Seek al inicio de pixel data
    err = fat32_seek(&f, bfOffBits);
    if (err != FAT32_OK) { fat32_close(&f); return false; }

    // Buffer de lectura de 1 fila cruda
    uint8_t row_raw[ (GFX_BMP_MAX_W * 3 + 3) & ~3 ];
    if (stride > sizeof(row_raw)) { fat32_close(&f); return false; }

    // Buffer para blit por segmentos (RGB565)
    uint16_t row565[GFX_BMP_MAX_W];

    // Para cada fila de imagen
    for (int row = 0; row < biHeight; row++)
    {
        // En BMP bottom-up, la primera fila en archivo es la de abajo.
        int src_row = top_down ? row : (biHeight - 1 - row);

        // Leer fila src_row: como estamos leyendo secuencial, hacemos seek por fila
        uint32_t row_pos = bfOffBits + (uint32_t)src_row * stride;
        err = fat32_seek(&f, row_pos);
        if (err != FAT32_OK) { fat32_close(&f); return false; }

        err = fat32_read(&f, row_raw, stride, &br);
        if (err != FAT32_OK || br != stride) { fat32_close(&f); return false; }

        // Convertir fila a RGB565
        if (biBitCount == 16)
        {
            // BMP suele ser little-endian: cada pixel = uint16_t LE
            for (int col = 0; col < biWidth; col++) {
                uint16_t px = (uint16_t)row_raw[col * 2] | ((uint16_t)row_raw[col * 2 + 1] << 8);
                row565[col] = px;
            }
        }
        else // 24 bpp (BGR)
        {
            for (int col = 0; col < biWidth; col++) {
                uint8_t b = row_raw[col * 3 + 0];
                uint8_t g = row_raw[col * 3 + 1];
                uint8_t r = row_raw[col * 3 + 2];
                row565[col] = bgr24_to_rgb565(b, g, r);
            }
        }

        // Dibujar con transparencia: bliteamos "runs" contiguos no transparentes
        int dst_y = y + row;
        if (dst_y < 0 || dst_y >= (int)HEIGHT) continue;


        // Dibujar la fila completa (SIN transparencia, para testear)
        lcd_blit(row565, (uint16_t)x, (uint16_t)dst_y, (uint16_t)biWidth, 1);
       
    }

    fat32_close(&f);
    return true;
}