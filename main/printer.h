#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    PRINTER_ALIGN_LEFT   = 0,
    PRINTER_ALIGN_CENTER = 1,
    PRINTER_ALIGN_RIGHT  = 2,
} printer_align_t;

typedef enum {
    PRINTER_SIZE_SMALL       = -1,   /* ESC M 1 — Font B (condensed) */
    PRINTER_SIZE_NORMAL      = 0x00,
    PRINTER_SIZE_DOUBLE_H    = 0x01,
    PRINTER_SIZE_DOUBLE_W    = 0x10,
    PRINTER_SIZE_DOUBLE_BOTH = 0x11,
} printer_size_t;

void printer_init(void);
void printer_reset(void);

void printer_write(const char *data, size_t len);
void printer_println(const char *text);

void printer_set_align(printer_align_t align);
void printer_set_bold(bool bold);
void printer_set_size(printer_size_t size);
void printer_set_underline(uint8_t weight);  /* 0=off 1=thin 2=thick */

void printer_feed(uint8_t lines);
void printer_cut(void);
