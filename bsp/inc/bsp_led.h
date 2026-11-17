#ifndef BSP_LED_H
#define BSP_LED_H

#include <stdint.h>

void bsp_led_init(void);
void bsp_led_on(void);
void bsp_led_off(void);
void bsp_led_toggle(void);

/* Lỗi nghiêm trọng: nháy LED PC13 "code" lần, nghỉ 1.5 s, lặp mãi (chờ bận, dùng được khi scheduler chưa chạy hoặc ngắt đã tắt).
 *  1 = app_ipc_init lỗi        2 = tạo TaskSensors lỗi      3 = tạo TaskProcessAlarm lỗi
 *  4 = tạo TaskDisplayUI lỗi   5 = vTaskStartScheduler trả về (thiếu heap cho Idle task)
 *  6 = tràn stack              7 = hết heap (malloc failed)
 *  9 = SPI HAL_ERROR  10 = SPI HAL_BUSY  11 = SPI HAL_TIMEOUT (phần cứng SPI1 truyền lỗi, chỉ khi APP_DEBUG_BRINGUP = 1) */
void bsp_led_fatal(uint8_t code) __attribute__((noreturn));

#endif
