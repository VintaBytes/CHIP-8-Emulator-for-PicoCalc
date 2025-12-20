//----------------------------------------------------
#include "rom_menu.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "drivers/console.h"
#include "drivers/fat32.h"
#include "keyboard.h"

#define MAX_ROMS   64
#define MAX_PATH   64
#define VIEW_ROWS  24

//----------------------------------------------------
static void copiar_str(char *dst, int dst_len, const char *src)
{
    if (dst_len <= 0) return;
    int i = 0;
    while (src && src[i] && i < dst_len - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}


//----------------------------------------------------
static bool termina_en_ch8(const char *s)
{
    int L = (int)strlen(s);
    if (L < 4) return false;
    char a = s[L-4], b = s[L-3], c = s[L-2], d = s[L-1];
    if (a != '.') return false;
    if ((b=='c'||b=='C') && (c=='h'||c=='H') && (d=='8')) return true;
    return false;
}


//----------------------------------------------------
static int leer_tecla_bloqueante(void)
{
    while (!keyboard_key_available()) { }
    return (uint8_t)keyboard_get_key();
}


//----------------------------------------------------
static void pausa_tecla(void)
{
    con_print("\n[tecla]\n");
    (void)leer_tecla_bloqueante();
}


//----------------------------------------------------
// Lista /roms y devuelve paths completos "/roms/XXXX.ch8"
static int scan_roms(char roms[MAX_ROMS][MAX_PATH])
{
    int n = 0;

    con_print("scan_roms(): abriendo /roms...\n");

    fat32_file_t dir;
    fat32_error_t err = fat32_open(&dir, "/roms");
    if (err != FAT32_OK) {
        con_print("fat32_open('/roms') ERROR: ");
        con_print(fat32_error_string(err));
        con_print("\n");
        pausa_tecla();
        return 0;
    }

    con_print("OK. Leyendo entradas...\n");

    int leidas = 0;

    while (n < MAX_ROMS) {
        fat32_entry_t e;
        err = fat32_dir_read(&dir, &e);

        if (err != FAT32_OK) {
            con_print("fat32_dir_read ERROR: ");
            con_print(fat32_error_string(err));
            con_print("\n");
            break;
        }

        leidas++;

        // Debug: mostrar primeras entradas aunque no sean .ch8
        // if (leidas <= 8) {
        //     con_print("  entry: '");
        //     con_print(e.filename);
        //     con_print("'  attr=");
        //     char buf[16];
        //     snprintf(buf, sizeof(buf), "%02X", (unsigned)e.attr);
        //     con_print(buf);
        //     con_print("\n");
        // }

        // Omitir directorios
        if (e.attr & FAT32_ATTR_DIRECTORY) {
            if (dir.last_entry_read) break;
            continue;
        }

        if (!termina_en_ch8(e.filename)) {
            if (dir.last_entry_read) break;
            continue;
        }

        char full[MAX_PATH];
        snprintf(full, sizeof(full), "/roms/%s", e.filename);

        copiar_str(roms[n], MAX_PATH, full);
        n++;

        if (dir.last_entry_read) break;
    }

    fat32_close(&dir);

    // con_print("scan_roms(): leidas=");
    // {
    //     char b[32];
    //     snprintf(b, sizeof(b), "%d", leidas);
    //     con_print(b);
    // }
    // con_print("  roms=");
    // {
    //     char b[32];
    //     snprintf(b, sizeof(b), "%d", n);
    //     con_print(b);
    // }

    return n;
}


//----------------------------------------------------
bool rom_menu_select(char *out_path, int out_path_len)
{
    // Forzar consola visible
    con_reset();
    con_clear();

    con_print("Menu ROMs...\n");

    static char roms[MAX_ROMS][MAX_PATH];
    int count = scan_roms(roms);

    if (count <= 0) {
        con_clear();
        con_print("No hay ROMs (.CH8) en /roms\n");
        con_print("o scan_roms fallo.\n\n");
        con_print("[ESC] salir | otra tecla: reintentar\n");

        while (1) {
            int k = leer_tecla_bloqueante();
            if (k == 0x1B) return false;
            // reintentar
            con_reset();
            con_clear();
            con_print("Reintentando...\n");
            count = scan_roms(roms);
            if (count > 0) break;
            con_clear();
            con_print("Sigue sin ROMs.\n");
            con_print("[ESC] salir | otra tecla: reintentar\n");
        }
    }

    int sel = 0;
    int top = 0;

    while (1) {
        con_clear();
        con_print("---------------------------------------\n");
        con_print("       CHIP-8 - Seleccionar ROM\n");
        con_print("---------------------------------------\n\n");

        int end = top + VIEW_ROWS;
        if (end > count) end = count;

        for (int i = top; i < end; i++) {

            // Color segun seleccion
            if (i == sel) {
                con_text_color(CON_BLACK, CON_WHITE);
            } else {
                con_text_color(CON_WHITE, CON_BLACK);  
            }

            // Truncar o rellenar a 40 caracteres
            const char *s = roms[i];
            char buf[41]; // 40 + '\0'
            int len = 0;

            // Calcular largo hasta 40
            while (s[len] != '\0' && len < 40) {
                buf[len] = s[len];
                len++;
            }

            if (s[len] == '\0') {
                // Cadena mas corta: rellenar con espacios
                for (int j = len; j < 39; j++) {
                    buf[j] = ' ';
                }
            } else {
                // Cadena mas larga: reemplazar ultimos 3 por "..."
                buf[37] = '.';
                buf[38] = '.';
                buf[39] = '.';
            }

            // Terminar string
            buf[39] = '\0';

            con_print(buf);
            con_print("\n");
        }

        // Volver a blanco para el resto de la UI
        con_text_color(CON_WHITE, CON_BLACK);  

        con_print("\n---------------------------------------\n");
        con_print("  UP/DOWN  | ENTER: Load  | ESC: Exit\n");

        int k = leer_tecla_bloqueante();

        if (k == 0xB5) { // UP
            if (sel > 0) sel--;
            if (sel < top) top = sel;
        }
        else if (k == 0xB6) { // DOWN
            if (sel < count - 1) sel++;
            if (sel >= top + VIEW_ROWS) top = sel - VIEW_ROWS + 1;
        }
        else if (k == 0x0D || k == 0x0A) { // ENTER
            copiar_str(out_path, out_path_len, roms[sel]);
            return true;
        }
        else if (k == 0x1B) { // ESC
            return false;
        }
    }
}
