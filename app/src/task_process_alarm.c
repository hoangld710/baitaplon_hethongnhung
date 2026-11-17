/* task_process_alarm.c - TaskProcessAlarm (Priority: Med)
 * Nhận Queue 1 -> quét nút chọn dải đo -> đọc DS1307 (I2C Mutex) -> so sánh quá tải -> đẩy Queue 2. */
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "task_process_alarm.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_button.h"
#include "hal_ds1307.h"

static TaskHandle_t xProcessTaskHandle = NULL;

static uint16_t prvSelectScale(uint8_t btn, uint16_t current)
{
    switch (btn) {
        case HAL_BTN_2KG: return APP_SCALE_2KG_G;
        case HAL_BTN_3KG: return APP_SCALE_3KG_G;
        case HAL_BTN_5KG: return APP_SCALE_5KG_G;
        default:          return current;       /* không nhấn, hoặc nhấn nhiều nút cùng lúc: giữ nguyên */
    }
}

/* Chiếm I2C Mutex (có timeout) -> đọc DS1307 -> nhả Mutex */
static bool prvReadRtc(RtcDateTime_t *dt)
{
    bool ok = false;

    if (xSemaphoreTake(xMutexI2C, pdMS_TO_TICKS(APP_I2C_MUTEX_TIMEOUT_MS)) == pdTRUE) {
        ok = hal_ds1307_read(dt);
        xSemaphoreGive(xMutexI2C);
    }
    return ok;
}

static void prvProcessTask(void *pvParameters)
{
    uint16_t       maxScale = APP_SCALE_DEFAULT_G;
    uint8_t        prevBtn  = 0;
    ScaleData_t    in       = {0, 0.0f};
    ScaleDisplay_t out;
    const TickType_t bootTick = xTaskGetTickCount();
    (void)pvParameters;

    memset(&out, 0, sizeof out);
    hal_button_init();

#if APP_RTC_SET_FROM_COMPILE_TIME
    if (xSemaphoreTake(xMutexI2C, pdMS_TO_TICKS(APP_I2C_MUTEX_TIMEOUT_MS)) == pdTRUE) {
        (void)hal_ds1307_set_compile_time();
        xSemaphoreGive(xMutexI2C);
    }
#endif

    for (;;) {
        g_stackHwm[1] = uxTaskGetStackHighWaterMark(NULL);

        /* 1. Nhận gói tin từ Queue 1 (có timeout để phát hiện mất cảm biến) */
        bool sensorOk = (xQueueReceive(xQueueSensor, &in,
                         pdMS_TO_TICKS(APP_PROCESS_SENSOR_TIMEOUT_MS)) == pdPASS);

        /* 2. Quét 3 nút. Chống dội: chỉ nhận khi 2 lần quét liên tiếp (~100 ms) giống nhau */
        uint8_t btn = hal_button_read();
        if (btn != 0U && btn == prevBtn) {
            maxScale = prvSelectScale(btn, maxScale);
        }
        prevBtn = btn;

        /* 3. Đọc ngày giờ từ DS1307 */
        out.rtc_ok = prvReadRtc(&out.dt);

        /* 4. So sánh dải đo & đóng gói */
        float g = (in.weight_g > 0.0f) ? in.weight_g : 0.0f;       /* không hiển thị khối lượng âm */
        out.weight_kg   = g / 1000.0f;
        out.max_scale_g = maxScale;
        out.sensor_ok   = sensorOk;
        out.starting    = !sensorOk && ((xTaskGetTickCount() - bootTick) < pdMS_TO_TICKS(APP_BOOT_GRACE_MS));
        out.overload    = sensorOk && (in.weight_g > (float)maxScale);
        out.raw_net     = in.raw_net;

        /* 5. Đẩy vào Queue 2 (mailbox: luôn giữ bản mới nhất, không bao giờ chặn) */
        (void)xQueueOverwrite(xQueueDisplay, &out);
    }
}

BaseType_t task_process_alarm_create(void)
{
    return xTaskCreate(prvProcessTask, "PROCESS", TASK_PROCESS_STACK,
                       NULL, TASK_PROCESS_PRIO, &xProcessTaskHandle);
}
