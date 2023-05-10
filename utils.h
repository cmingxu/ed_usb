#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include "./include/ftd3xx.h"

void _pack_char_arr(char *, const char *, size_t);
void _pack_uint8_arr(uint8_t *, const uint8_t *, size_t );
void _pack_uint32(uint8_t *, uint32_t);
void _pack_int32(uint8_t *, int32_t);
void _pack_uint16(uint8_t *, uint16_t);
void _pack_short(uint8_t *, short);

void _debug_hex(void *, size_t);


#endif
