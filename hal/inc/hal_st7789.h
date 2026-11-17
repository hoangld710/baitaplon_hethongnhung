#ifndef HAL_ST7789_H
#define HAL_ST7789_H

#include <stdint.h>
#include <stdbool.h>

/* ====== Cấu hình panel: LCD IPS 1.3" 240x240, IC ST7789 ====== */
#define ST7789_WIDTH      240
#define ST7789_HEIGHT     240
#define ST7789_XSTART     0
#define ST7789_YSTART     0
#define ST7789_MADCTL_VAL 0x00       /* 0x08 nếu đỏ <-> xanh bị đảo màu */

/* 1: module có chân CS (8 chân) và đã nối vào TFT_CS (PA4).
 * 0: module 7 chân KHÔNG có CS (phổ biến nhất): driver không điều khiển CS. */
#ifndef ST7789_USE_CS
#define ST7789_USE_CS     0
#endif

/* RGB565 */
#define ST7789_BLACK      0x0000
#define ST7789_WHITE      0xFFFF
#define ST7789_RED        0xF800
#define ST7789_GREEN      0x07E0
#define ST7789_BLUE       0x001F
#define ST7789_YELLOW     0xFFE0
#define ST7789_CYAN       0x07FF
#define ST7789_GRAY       0x8410

/* Các hàm dưới đây KHÔNG tự khóa mutex: task gọi phải giữ xMutexSPI. */
uint8_t st7789_spi_error(void);      /* 0 = chưa có lỗi; 1 = HAL_ERROR, 2 = HAL_BUSY, 3 = HAL_TIMEOUT (lỗi đầu tiên của HAL_SPI_Transmit) */
bool st7789_init(void);              /* gọi trong ngữ cảnh task (có dùng vTaskDelay) */
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void st7789_fill_screen(uint16_t color);
void st7789_draw_string(uint16_t x, uint16_t y, const char *s,
                        uint16_t fg, uint16_t bg, uint8_t scale);   /* scale 1..8, nền đặc */

#endif
