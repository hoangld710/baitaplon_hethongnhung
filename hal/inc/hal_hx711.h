#ifndef HAL_HX711_H
#define HAL_HX711_H

#include <stdint.h>
#include <stdbool.h>

void hal_hx711_init(void);
bool hal_hx711_is_ready(void);              /* DT xuống thấp = có dữ liệu mới */
bool hal_hx711_read_raw(int32_t *raw);      /* đọc 24-bit có dấu, kênh A gain 128; false nếu chưa sẵn sàng */

#endif
