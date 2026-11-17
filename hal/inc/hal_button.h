#ifndef HAL_BUTTON_H
#define HAL_BUTTON_H

#include <stdint.h>

#define HAL_BTN_2KG   (1U << 0)
#define HAL_BTN_3KG   (1U << 1)
#define HAL_BTN_5KG   (1U << 2)

void    hal_button_init(void);
uint8_t hal_button_read(void);              /* bitmask các nút đang được nhấn */

#endif
