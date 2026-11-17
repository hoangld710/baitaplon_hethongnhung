/* task_sensors.c - TaskSensors (Priority: High)
 * Đọc HX711 -> tare tự động lúc khởi động -> lọc trung bình động -> đẩy ScaleData_t vào Queue 1. */
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "task_sensors.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_hx711.h"

static TaskHandle_t xSensorsTaskHandle = NULL;

/* ---------- Lọc trung bình động (Moving Average) ---------- */
typedef struct {
    int32_t buf[APP_MAVG_WINDOW];
    int64_t sum;
    uint8_t idx;
    uint8_t count;
} MovAvg_t;

static void prvMavgReset(MovAvg_t *m)
{
    memset(m, 0, sizeof *m);
}

static int32_t prvMavgUpdate(MovAvg_t *m, int32_t x)
{
    if (m->count < APP_MAVG_WINDOW) {
        m->count++;
    } else {
        m->sum -= m->buf[m->idx];       /* bỏ mẫu cũ nhất ra khỏi cửa sổ */
    }
    m->buf[m->idx] = x;
    m->sum += x;
    m->idx = (uint8_t)((m->idx + 1U) % APP_MAVG_WINDOW);
    return (int32_t)(m->sum / m->count);
}

/* ---------- Đọc 1 mẫu: chờ DT xuống thấp có timeout, không chặn CPU ---------- */
static bool prvReadRaw(int32_t *raw)
{
    const TickType_t start   = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(APP_HX711_READY_TIMEOUT_MS);

    while (!hal_hx711_is_ready()) {
        if ((xTaskGetTickCount() - start) >= timeout) {
            return false;
        }
        vTaskDelay(1);
    }
    {
        bool ok = hal_hx711_read_raw(raw);
        /* Luôn nhường CPU sau mỗi lần đọc. Nếu thiếu, khi DT kẹt mức thấp (HX711 chưa nối/hỏng) vòng lặp này
         * chạy không nghỉ và, vì là task ưu tiên CAO NHẤT, làm đói mọi task thấp hơn (màn hình đứng hình). */
        vTaskDelay(pdMS_TO_TICKS(APP_SENSOR_MIN_GAP_MS));
        return ok;
    }
}

/* ---------- Tare tự động: trung bình APP_TARE_SAMPLES mẫu (sàn cân phải trống lúc bật máy) ---------- */
static bool prvTare(int32_t *offset)
{
    int64_t sum = 0;
    uint8_t good = 0, tries = 0;
    int32_t r;

    /* Bỏ vài mẫu đầu sau khi bật vì chưa ổn định */
    for (uint8_t i = 0; i < 3U; i++) { (void)prvReadRaw(&r); }

    while (good < APP_TARE_SAMPLES && tries < (uint8_t)(APP_TARE_SAMPLES * 3U)) {
        tries++;
        if (prvReadRaw(&r)) {
            sum += r;
            good++;
        }
    }
    if (good < APP_TARE_SAMPLES) {
        return false;
    }
    *offset = (int32_t)(sum / good);
    return true;
}

static void prvSensorsTask(void *pvParameters)
{
    MovAvg_t mavg;
    int32_t  offset = 0;
    bool     tared  = false;
    int32_t  raw;
    (void)pvParameters;

    hal_hx711_init();
    prvMavgReset(&mavg);
    vTaskDelay(pdMS_TO_TICKS(APP_HX711_SETTLE_MS));

    for (;;) {
        g_stackHwm[0] = uxTaskGetStackHighWaterMark(NULL);

        if (!tared) {
            tared = prvTare(&offset);
            if (!tared) {
                vTaskDelay(pdMS_TO_TICKS(200));     /* chưa thấy HX711: thử lại */
            }
            continue;
        }

        if (!prvReadRaw(&raw)) {
            continue;       /* timeout: không gửi gói => ProcessAlarm sẽ báo lỗi cảm biến */
        }

        ScaleData_t pkt;
        pkt.raw_net  = prvMavgUpdate(&mavg, raw - offset);
        pkt.weight_g = (float)pkt.raw_net / APP_HX711_COUNTS_PER_GRAM;

        /* Queue đầy: bỏ gói mới (ProcessAlarm đang chậm), không chặn task ưu tiên cao */
        (void)xQueueSend(xQueueSensor, &pkt, 0);
    }
}

BaseType_t task_sensors_create(void)
{
    return xTaskCreate(prvSensorsTask, "SENSORS", TASK_SENSORS_STACK,
                       NULL, TASK_SENSORS_PRIO, &xSensorsTaskHandle);
}
