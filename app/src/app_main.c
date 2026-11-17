/* app_main.c - Điểm vào ứng dụng, được gọi từ main.c (USER CODE BEGIN 2) sau khi CubeMX init xong */
#include "app_main.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "bsp_delay.h"
#include "bsp_led.h"
#include "task_sensors.h"
#include "task_process_alarm.h"
#include "task_display_ui.h"

void app_main(void)
{
    /* 1. Hardware init (HAL của CubeMX đã init xong clock/I2C/SPI) */
    bsp_delay_init();
    bsp_led_init();

    /* 2. Tạo đối tượng IPC. Lỗi => LED PC13 nháy theo mã (xem bsp_led.h) */
    if (app_ipc_init()              != pdPASS) { bsp_led_fatal(1); }

    /* 3. Tạo Task (bước Skeleton: comment 3 dòng này; bước Integration: bỏ comment) */
    if (task_sensors_create()       != pdPASS) { bsp_led_fatal(2); }
    if (task_process_alarm_create() != pdPASS) { bsp_led_fatal(3); }
    if (task_display_ui_create()    != pdPASS) { bsp_led_fatal(4); }

    /* 4. Chạy scheduler */
    vTaskStartScheduler();

    /* Chỉ tới đây nếu không đủ heap cho Idle task */
    bsp_led_fatal(5);
}
