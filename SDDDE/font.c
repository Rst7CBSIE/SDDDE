#include <proto/exec.h>

const UBYTE font[2048]=
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00, 
  0x00, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x24, 0x7E, 0x24, 0x24, 0x7E, 0x24, 0x00, 
  0x00, 0x18, 0x3E, 0x58, 0x3C, 0x1A, 0x7C, 0x18, 
  0x00, 0x62, 0x64, 0x08, 0x10, 0x26, 0x46, 0x00, 
  0x00, 0x10, 0x28, 0x10, 0x2A, 0x66, 0x3A, 0x00, 
  0x00, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x0C, 0x18, 0x18, 0x18, 0x18, 0x0C, 0x00, 
  0x00, 0x30, 0x18, 0x18, 0x18, 0x18, 0x30, 0x00, 
  0x00, 0x00, 0x24, 0x18, 0x7E, 0x18, 0x24, 0x00, 
  0x00, 0x00, 0x10, 0x10, 0x7C, 0x10, 0x10, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 
  0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 
  0x00, 0x00, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00, 
  0x00, 0x3C, 0x66, 0x6E, 0x76, 0x66, 0x3C, 0x00, 
  0x00, 0x18, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00, 
  0x00, 0x3C, 0x66, 0x06, 0x3C, 0x60, 0x7E, 0x00, 
  0x00, 0x3C, 0x66, 0x0C, 0x06, 0x66, 0x3C, 0x00, 
  0x00, 0x0C, 0x1C, 0x2C, 0x4C, 0x7E, 0x0C, 0x00, 
  0x00, 0x7E, 0x60, 0x7C, 0x06, 0x66, 0x3C, 0x00, 
  0x00, 0x3C, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x7E, 0x66, 0x0C, 0x18, 0x18, 0x18, 0x00, 
  0x00, 0x3C, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x3C, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x30, 
  0x00, 0x00, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x00, 
  0x00, 0x00, 0x00, 0x3E, 0x00, 0x3E, 0x00, 0x00, 
  0x00, 0x00, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x00, 
  0x00, 0x3C, 0x66, 0x0C, 0x18, 0x00, 0x18, 0x00, 
  0x00, 0x3C, 0x4A, 0x56, 0x5E, 0x40, 0x3C, 0x00, 
  0x00, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00, 
  0x00, 0x7C, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00, 
  0x00, 0x3C, 0x66, 0x60, 0x60, 0x66, 0x3C, 0x00, 
  0x00, 0x78, 0x6C, 0x66, 0x66, 0x6C, 0x78, 0x00, 
  0x00, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00, 
  0x00, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00, 
  0x00, 0x3C, 0x66, 0x60, 0x6E, 0x66, 0x3C, 0x00, 
  0x00, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00, 
  0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 
  0x00, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x66, 0x6C, 0x78, 0x78, 0x6C, 0x66, 0x00, 
  0x00, 0x60, 0x60, 0x60, 0x60, 0x66, 0x7E, 0x00, 
  0x00, 0x42, 0x66, 0x7E, 0x7E, 0x66, 0x66, 0x00, 
  0x00, 0x46, 0x66, 0x76, 0x6E, 0x66, 0x62, 0x00, 
  0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x00, 
  0x00, 0x3C, 0x66, 0x66, 0x76, 0x6E, 0x3C, 0x04, 
  0x00, 0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x00, 
  0x00, 0x3C, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00, 
  0x00, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 
  0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00, 
  0x00, 0x66, 0x66, 0x66, 0x7E, 0x7E, 0x24, 0x00, 
  0x00, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x66, 0x00, 
  0x00, 0x42, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00, 
  0x00, 0x7E, 0x4E, 0x1C, 0x38, 0x72, 0x7E, 0x00, 
  0x00, 0x1E, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00, 
  0x00, 0x00, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00, 
  0x00, 0x78, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00, 
  0x00, 0x18, 0x3C, 0x5A, 0x18, 0x18, 0x18, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 
  0x00, 0x1C, 0x36, 0x30, 0x78, 0x30, 0x7E, 0x00, 
  0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00, 
  0x00, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x7C, 0x00, 
  0x00, 0x00, 0x3C, 0x64, 0x60, 0x64, 0x3C, 0x00, 
  0x00, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3E, 0x00, 
  0x00, 0x00, 0x3C, 0x66, 0x7C, 0x60, 0x3E, 0x00, 
  0x00, 0x1C, 0x30, 0x38, 0x30, 0x30, 0x30, 0x00, 
  0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C, 
  0x00, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x00, 
  0x00, 0x18, 0x00, 0x38, 0x18, 0x18, 0x3C, 0x00, 
  0x00, 0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0x2C, 0x18, 
  0x00, 0x60, 0x6C, 0x78, 0x78, 0x6C, 0x66, 0x00, 
  0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x1C, 0x00, 
  0x00, 0x00, 0x74, 0x7E, 0x6A, 0x6A, 0x6A, 0x00, 
  0x00, 0x00, 0x78, 0x6C, 0x6C, 0x6C, 0x6C, 0x00, 
  0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 
  0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x07, 
  0x00, 0x00, 0x1C, 0x30, 0x30, 0x30, 0x30, 0x00, 
  0x00, 0x00, 0x38, 0x60, 0x38, 0x0C, 0x78, 0x00, 
  0x00, 0x18, 0x3C, 0x18, 0x18, 0x18, 0x0C, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x3C, 0x3C, 0x18, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x7E, 0x7E, 0x24, 0x00, 
  0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C, 
  0x00, 0x00, 0x7E, 0x4C, 0x18, 0x32, 0x7E, 0x00, 
  0x00, 0x1E, 0x18, 0x30, 0x18, 0x18, 0x1E, 0x00, 
  0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 
  0x00, 0x78, 0x18, 0x0C, 0x18, 0x18, 0x78, 0x00, 
  0x00, 0x26, 0x7C, 0x48, 0x00, 0x00, 0x00, 0x00, 
  0x3C, 0x42, 0x9D, 0xB1, 0xB1, 0x9D, 0x42, 0x3C, 
  0x00, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00, 
  0x00, 0x7E, 0x60, 0x7C, 0x66, 0x66, 0x7C, 0x00, 
  0x00, 0x7C, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00, 
  0x00, 0x7E, 0x66, 0x60, 0x60, 0x60, 0x60, 0x00, 
  0x00, 0x3E, 0x66, 0x66, 0x66, 0x66, 0xFF, 0xC3, 
  0x00, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00, 
  0x00, 0xDB, 0x5A, 0x3C, 0x3C, 0x5A, 0xDB, 0x00, 
  0x00, 0x3C, 0x66, 0x0C, 0x06, 0x66, 0x3C, 0x00, 
  0x00, 0x62, 0x66, 0x6E, 0x76, 0x66, 0x46, 0x00, 
  0x08, 0x6A, 0x66, 0x6E, 0x76, 0x66, 0x46, 0x00, 
  0x00, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x66, 0x00, 
  0x00, 0x3E, 0x66, 0x66, 0x66, 0x66, 0xC6, 0x00, 
  0x00, 0x42, 0x66, 0x7E, 0x7E, 0x66, 0x66, 0x00, 
  0x00, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00, 
  0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x7E, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 
  0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x00, 
  0x00, 0x3C, 0x66, 0x60, 0x60, 0x66, 0x3C, 0x00, 
  0x00, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 
  0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C, 0x00, 
  0x00, 0x7E, 0xDB, 0xDB, 0xDB, 0x7E, 0x18, 0x00, 
  0x00, 0x66, 0x7E, 0x18, 0x18, 0x7E, 0x66, 0x00, 
  0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFE, 0x06, 
  0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x00, 
  0x00, 0xC6, 0xD6, 0xD6, 0xD6, 0xD6, 0xFE, 0x00, 
  0x00, 0xC6, 0xD6, 0xD6, 0xD6, 0xD6, 0xFF, 0x01, 
  0x00, 0xE0, 0x60, 0x7C, 0x66, 0x66, 0x7C, 0x00, 
  0x00, 0xC6, 0xC6, 0xF6, 0xDA, 0xDA, 0xF6, 0x00, 
  0x00, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x7C, 0x00, 
  0x00, 0x3C, 0x66, 0x1E, 0x06, 0x66, 0x3C, 0x00, 
  0x00, 0xDC, 0xB6, 0xF6, 0xB6, 0xB6, 0xDC, 0x00, 
  0x00, 0x3E, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x00, 
  0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00, 
  0x00, 0x00, 0x7C, 0x60, 0x7C, 0x66, 0x7C, 0x00, 
  0x00, 0x00, 0x7C, 0x66, 0x7C, 0x66, 0x7C, 0x00, 
  0x00, 0x00, 0x7C, 0x64, 0x60, 0x60, 0x60, 0x00, 
  0x00, 0x00, 0x3C, 0x6C, 0x6C, 0x6C, 0xFE, 0x82, 
  0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3E, 0x00, 
  0x00, 0x00, 0xDB, 0x5A, 0x3C, 0x5A, 0xDB, 0x00, 
  0x00, 0x00, 0x3C, 0x66, 0x0C, 0x66, 0x3C, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x6E, 0x76, 0x66, 0x00, 
  0x18, 0x10, 0x66, 0x66, 0x6E, 0x76, 0x66, 0x00, 
  0x00, 0x00, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00, 
  0x00, 0x00, 0x1E, 0x36, 0x36, 0x36, 0x66, 0x00, 
  0x00, 0x00, 0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00, 
  0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00, 
  0x00, 0x00, 0x7E, 0x66, 0x66, 0x66, 0x66, 0x00, 
  0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 
  0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 
  0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
  0x10, 0x10, 0x10, 0x10, 0xF0, 0x10, 0x10, 0x10, 
  0x10, 0x10, 0xF0, 0x10, 0x10, 0xF0, 0x10, 0x10, 
  0x24, 0x24, 0x24, 0x24, 0xE4, 0x24, 0x24, 0x24, 
  0x00, 0x00, 0x00, 0x00, 0xFC, 0x24, 0x24, 0x24, 
  0x00, 0x00, 0xF0, 0x10, 0x10, 0xF0, 0x10, 0x10, 
  0x24, 0x24, 0xE4, 0x04, 0x04, 0xE4, 0x24, 0x24, 
  0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
  0x00, 0x00, 0xFC, 0x04, 0x04, 0xE4, 0x24, 0x24, 
  0x24, 0x24, 0xE4, 0x04, 0x04, 0xFC, 0x00, 0x00, 
  0x24, 0x24, 0x24, 0x24, 0xFC, 0x00, 0x00, 0x00, 
  0x10, 0x10, 0xF0, 0x10, 0x10, 0xF0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF0, 0x10, 0x10, 0x10, 
  0x10, 0x10, 0x10, 0x10, 0x1F, 0x00, 0x00, 0x00, 
  0x10, 0x10, 0x10, 0x10, 0xFF, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xFF, 0x10, 0x10, 0x10, 
  0x10, 0x10, 0x10, 0x10, 0x1F, 0x10, 0x10, 0x10, 
  0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
  0x10, 0x10, 0x10, 0x10, 0xFF, 0x10, 0x10, 0x10, 
  0x00, 0x00, 0x1F, 0x10, 0x10, 0x1F, 0x10, 0x10, 
  0x24, 0x24, 0x24, 0x24, 0x27, 0x24, 0x24, 0x24, 
  0x24, 0x24, 0x27, 0x20, 0x20, 0x3F, 0x00, 0x00, 
  0x00, 0x00, 0x3F, 0x20, 0x20, 0x27, 0x24, 0x24, 
  0x24, 0x24, 0xE7, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x00, 0x00, 0xFF, 0x00, 0x00, 0xE7, 0x24, 0x24, 
  0x24, 0x24, 0x27, 0x20, 0x20, 0x27, 0x24, 0x24, 
  0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x24, 0x24, 0xE7, 0x00, 0x00, 0xE7, 0x24, 0x24, 
  0x10, 0x10, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 
  0x24, 0x24, 0x24, 0x24, 0xFF, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x10, 0x10, 
  0x00, 0x00, 0x00, 0x00, 0xFF, 0x24, 0x24, 0x24, 
  0x24, 0x24, 0x24, 0x24, 0x3F, 0x00, 0x00, 0x00, 
  0x10, 0x10, 0x1F, 0x10, 0x10, 0x1F, 0x00, 0x00, 
  0x00, 0x00, 0x1F, 0x10, 0x10, 0x1F, 0x10, 0x10, 
  0x00, 0x00, 0x00, 0x00, 0x3F, 0x24, 0x24, 0x24, 
  0x24, 0x24, 0x24, 0x24, 0xFF, 0x24, 0x24, 0x24, 
  0x10, 0x10, 0xFF, 0x10, 0x10, 0xFF, 0x10, 0x10, 
  0x10, 0x10, 0x10, 0x10, 0xF0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x1F, 0x10, 0x10, 0x10, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
  0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x00, 
  0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00, 
  0x00, 0x00, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x3E, 0x06, 0x3C, 0x00, 
  0x00, 0x00, 0x7E, 0xDB, 0xDB, 0x7E, 0x18, 0x00, 
  0x00, 0x00, 0x66, 0x7E, 0x18, 0x7E, 0x66, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x7F, 0x03, 
  0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x00, 
  0x00, 0x00, 0xC6, 0xD6, 0xD6, 0xD6, 0xFE, 0x00, 
  0x00, 0x00, 0xC6, 0xD6, 0xD6, 0xD6, 0xFF, 0x01, 
  0x00, 0x00, 0xE0, 0x7C, 0x66, 0x66, 0x7C, 0x00, 
  0x00, 0x00, 0xC6, 0xF6, 0xDA, 0xDA, 0xF6, 0x00, 
  0x00, 0x00, 0x60, 0x7C, 0x66, 0x66, 0x7C, 0x00, 
  0x00, 0x00, 0x7C, 0x46, 0x1E, 0x46, 0x7C, 0x00, 
  0x00, 0x00, 0xDC, 0xB6, 0xF6, 0xB6, 0xDC, 0x00, 
  0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x66, 0x00, 
  0x24, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00, 
  0x00, 0x24, 0x3C, 0x66, 0x7E, 0x60, 0x3E, 0x00, 
  0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x00, 
  0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x00, 
  0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x00, 
  0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x00, 
  0x00, 0x00, 0x08, 0x0C, 0x7E, 0x0C, 0x08, 0x00, 
  0x00, 0x00, 0x10, 0x30, 0x7E, 0x30, 0x10, 0x00, 
  0x00, 0x10, 0x38, 0x7C, 0x10, 0x10, 0x10, 0x00, 
  0x00, 0x10, 0x10, 0x10, 0x7C, 0x38, 0x10, 0x00, 
  0x00, 0x00, 0x10, 0x00, 0x7C, 0x00, 0x10, 0x00, 
  0x00, 0x10, 0x10, 0x7C, 0x10, 0x10, 0x7C, 0x00, 
  0x00, 0x9E, 0xDE, 0xF8, 0xF8, 0xD8, 0xD8, 0x00, 
  0x00, 0x00, 0x44, 0x38, 0x28, 0x38, 0x44, 0x00, 
  0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};
