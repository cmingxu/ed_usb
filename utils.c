#include "ed.h"
#include "utils.h"

void _pack_uint32(uint8_t *pos, uint32_t value){
  size_t size = 4;
  for (int i = 0; i < size; i++) {
    *(pos+i) = value >> (i * 8) & 0xFF;
  }
}

void _pack_int32(uint8_t *pos, int32_t value){
  size_t size = 4;
  for (int i = 0; i < size; i++) {
    *(pos+i) = value >> (i * 8) & 0xFF;
  }
}

void _pack_uint16(uint8_t *pos, uint16_t value){
  size_t size = 2;
  for (int i = 0; i < size; i++) {
    *(pos+i) = value >> (i * 8) & 0xFF;
  }
}

void _pack_short(uint8_t *pos, short value){
  size_t size = 1;
  for (int i = 0; i < size; i++) {
    *(pos+i) = value >> (i * 8) & 0xFF;
  }
}

void _pack_char_arr(char *target, const char *src, size_t size){
  for (int i = 0; i < size; i++) {
    *(target+i) = src[size-i-1];
  }
}

void _pack_uint8_arr(uint8_t *target, const uint8_t *src, size_t size){
  for (size_t i = 0; i < size; i++) {
    *(target + i) =  src[i];
  }
}


void _debug_hex(void *buf, size_t size){
#ifdef ED_DEBUG
  char *tmp = (char *)buf;
  for (int i = 0; i < size; i++) {
    if ((i % 8 == 0)) {
      printf("\n");
    }
    printf("%02X ",  *(tmp+i) & 0xff);
  }

  printf("\n");
#endif
}


