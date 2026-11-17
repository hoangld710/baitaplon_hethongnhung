#ifndef BSP_DELAY_H
#define BSP_DELAY_H

#include <stdint.h>

void bsp_delay_init(void);          /* bật bộ đếm chu kỳ DWT (gọi 1 lần trước scheduler) */
void bsp_delay_us(uint32_t us);     /* trễ micro giây (busy-wait, chỉ dùng cho thời gian rất ngắn) */
void bsp_delay_ms(uint32_t ms);     /* trễ ms bằng vTaskDelay; CHỈ gọi trong Task */

#endif
