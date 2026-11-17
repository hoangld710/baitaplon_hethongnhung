#ifndef HAL_FONT5X7_H
#define HAL_FONT5X7_H

#include <stdint.h>

#define FONT5X7_FIRST   0x20    /* ' ' */
#define FONT5X7_LAST    0x5A    /* 'Z' (chữ thường được đổi sang chữ hoa khi vẽ) */

extern const uint8_t font5x7[(FONT5X7_LAST - FONT5X7_FIRST + 1) * 5];

#endif
