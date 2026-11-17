/* task_display_ui.c - TaskDisplayUI (Priority: Low)
 * Nhận Queue 2 -> chiếm SPI Mutex -> vẽ TFT ST7789 240x240 (chỉ vẽ lại phần thay đổi) -> nhả Mutex -> vTaskDelay. */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "task_display_ui.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_st7789.h"
#include "bsp_led.h"

/* ---- Bố cục màn 240x240 (font 5x7, mỗi ký tự rộng 6*scale, cao 8*scale) ---- */
#define UI_HEADER_H      28
#define UI_WEIGHT_X      8
#define UI_WEIGHT_Y      40          /* scale 6: "0.00" rộng 144 px, cao 48 px */
#define UI_UNIT_X        164
#define UI_UNIT_Y        64          /* "KG" scale 3, căn đáy với số */
#define UI_MODE_X        8
#define UI_MODE_Y        104         /* scale 2 */
#define UI_BAR_X         8
#define UI_BAR_Y         132
#define UI_BAR_W         224
#define UI_BAR_H         24
#define UI_STATUS_X      8
#define UI_STATUS_Y      168         /* scale 3: 9 ký tự = 162 px */
#define UI_RAW_X         8
#define UI_RAW_Y         198         /* scale 2 */
#define UI_DT_X          6
#define UI_DT_Y          220         /* scale 2: 19 ký tự = 228 px */

#define STATUS_NORMAL    0
#define STATUS_OVERLOAD  1
#define STATUS_NOSENSOR  2
#define STATUS_STARTING  3

static TaskHandle_t xDisplayTaskHandle = NULL;

/* Cache nội dung đã vẽ để chỉ vẽ lại khi thay đổi (giảm nhấp nháy và tải SPI) */
typedef struct {
    char     weight[8];
    char     mode[20];
    char     raw[24];
    char     dt[24];
    int16_t  barFill;
    uint16_t barColor;
    uint8_t  status;
} UiCache_t;

static UiCache_t ui;

static void prvCacheReset(void)
{
    memset(&ui, 0, sizeof ui);
    ui.barFill = -1;
    ui.status  = 0xFFU;
}

static bool prvChanged(char *cache, size_t cap, const char *s)
{
    if (strncmp(cache, s, cap) == 0) { return false; }
    strncpy(cache, s, cap - 1U);
    cache[cap - 1U] = '\0';
    return true;
}

/* Khung tĩnh: vẽ 1 lần */
static void prvDrawFrame(void)
{
    st7789_fill_screen(ST7789_BLACK);
    st7789_fill_rect(0, 0, ST7789_WIDTH, UI_HEADER_H, ST7789_BLUE);
    st7789_draw_string(8, 6, "SMART SCALE", ST7789_WHITE, ST7789_BLUE, 2);
    st7789_draw_string(UI_UNIT_X, UI_UNIT_Y, "KG", ST7789_YELLOW, ST7789_BLACK, 3);

    /* Viền thanh tải (đồ thị thanh) */
    st7789_fill_rect(UI_BAR_X, UI_BAR_Y, UI_BAR_W, UI_BAR_H, ST7789_WHITE);
    st7789_fill_rect(UI_BAR_X + 2, UI_BAR_Y + 2, UI_BAR_W - 4, UI_BAR_H - 4, ST7789_BLACK);
    prvCacheReset();
}

/* Khối lượng kg, làm tròn 2 chữ số thập phân, không dùng printf float */
static void prvFormatWeight(char *buf, size_t n, float kg)
{
    uint32_t centi = (uint32_t)(kg * 100.0f + 0.5f);
    if (centi > 999U) { centi = 999U; }                     /* tối đa hiển thị 9.99 */
    (void)snprintf(buf, n, "%lu.%02lu", (unsigned long)(centi / 100U), (unsigned long)(centi % 100U));
}

static void prvUpdate(const ScaleDisplay_t *d)
{
    char s[28];

    /* 1. Số kg thực tế */
    prvFormatWeight(s, sizeof s, d->weight_kg);
    if (prvChanged(ui.weight, sizeof ui.weight, s)) {
        st7789_draw_string(UI_WEIGHT_X, UI_WEIGHT_Y, s, ST7789_WHITE, ST7789_BLACK, 6);
    }

    /* 2. Chế độ dải đo hiện tại */
    (void)snprintf(s, sizeof s, "MODE: MAX %uKG", (unsigned)(d->max_scale_g / 1000U));
    if (prvChanged(ui.mode, sizeof ui.mode, s)) {
        st7789_draw_string(UI_MODE_X, UI_MODE_Y, s, ST7789_CYAN, ST7789_BLACK, 2);
    }

    /* 3. Thanh tải: tỉ lệ khối lượng / dải đo, xanh -> vàng -> đỏ */
    {
        const uint16_t innerW = UI_BAR_W - 4U;
        float ratio = (d->weight_kg * 1000.0f) / (float)d->max_scale_g;
        int16_t fill;
        uint16_t color;

        if (ratio > 1.0f) { ratio = 1.0f; }
        fill  = (int16_t)(ratio * (float)innerW);
        color = (ratio < 0.8f) ? ST7789_GREEN : ((d->overload || ratio >= 1.0f) ? ST7789_RED : ST7789_YELLOW);

        if (fill != ui.barFill || color != ui.barColor) {
            if (fill > 0) {
                st7789_fill_rect(UI_BAR_X + 2, UI_BAR_Y + 2, (uint16_t)fill, UI_BAR_H - 4, color);
            }
            if (fill < (int16_t)innerW) {
                st7789_fill_rect((uint16_t)(UI_BAR_X + 2 + fill), UI_BAR_Y + 2,
                                 (uint16_t)(innerW - (uint16_t)fill), UI_BAR_H - 4, ST7789_BLACK);
            }
            ui.barFill  = fill;
            ui.barColor = color;
        }
    }

    /* 4. Trạng thái (đều đủ 9 ký tự để ghi đè sạch): đang khởi động > mất cảm biến > quá tải > bình thường */
    {
        uint8_t st = d->starting ? STATUS_STARTING
                   : (!d->sensor_ok ? STATUS_NOSENSOR
                   : (d->overload ? STATUS_OVERLOAD : STATUS_NORMAL));
        if (st != ui.status) {
            switch (st) {
                case STATUS_STARTING: st7789_draw_string(UI_STATUS_X, UI_STATUS_Y, "STARTING ", ST7789_CYAN,   ST7789_BLACK, 3); break;
                case STATUS_NOSENSOR: st7789_draw_string(UI_STATUS_X, UI_STATUS_Y, "NO SENSOR", ST7789_YELLOW, ST7789_BLACK, 3); break;
                case STATUS_OVERLOAD: st7789_draw_string(UI_STATUS_X, UI_STATUS_Y, "OVERLOAD!", ST7789_RED,    ST7789_BLACK, 3); break;
                default:              st7789_draw_string(UI_STATUS_X, UI_STATUS_Y, "NORMAL   ", ST7789_GREEN,  ST7789_BLACK, 3); break;
            }
            ui.status = st;
        }
    }

#if APP_DEBUG_SHOW_RAW
    (void)snprintf(s, sizeof s, "RAW:%-12ld", (long)d->raw_net);
    if (prvChanged(ui.raw, sizeof ui.raw, s)) {
        st7789_draw_string(UI_RAW_X, UI_RAW_Y, s, ST7789_GRAY, ST7789_BLACK, 2);
    }
#endif

    /* 5. Mốc thời gian thực từ DS1307 */
    if (d->rtc_ok) {
        (void)snprintf(s, sizeof s, "%02u/%02u/%04u %02u:%02u:%02u",
                       (unsigned)d->dt.date, (unsigned)d->dt.month, (unsigned)d->dt.year,
                       (unsigned)d->dt.hour, (unsigned)d->dt.min,   (unsigned)d->dt.sec);
    } else {
        (void)snprintf(s, sizeof s, "--/--/---- --:--:--");
    }
    if (prvChanged(ui.dt, sizeof ui.dt, s)) {
        st7789_draw_string(UI_DT_X, UI_DT_Y, s, ST7789_WHITE, ST7789_BLACK, 2);
    }
}

static void prvDisplayTask(void *pvParameters)
{
    ScaleDisplay_t d;
    (void)pvParameters;

    /* Khởi tạo màn hình (dùng vTaskDelay bên trong nên phải chạy trong task) */
    while (xSemaphoreTake(xMutexSPI, pdMS_TO_TICKS(APP_SPI_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#if APP_DEBUG_BRINGUP
    bsp_led_on();                                   /* LED sáng: scheduler chạy và task màn hình đã bắt đầu */
#endif
    (void)st7789_init();
#if APP_DEBUG_BRINGUP
    {   /* Thử màn hình: nếu thấy 4 màu lần lượt thì SPI/DC/RST/CS/BLK đều đúng */
        static const uint16_t cols[4] = { ST7789_RED, ST7789_GREEN, ST7789_BLUE, ST7789_WHITE };
        for (uint8_t i = 0; i < 4U; i++) {
            st7789_fill_screen(cols[i]);
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }
    if (st7789_spi_error() != 0U) {
        bsp_led_fatal((uint8_t)(8U + st7789_spi_error()));      /* 9 = HAL_ERROR, 10 = HAL_BUSY, 11 = HAL_TIMEOUT */
    }
    bsp_led_off();
#endif
    prvDrawFrame();
    xSemaphoreGive(xMutexSPI);

    for (;;) {
        g_stackHwm[2] = uxTaskGetStackHighWaterMark(NULL);

        /* Nhận gói tin từ Queue 2 */
        if (xQueueReceive(xQueueDisplay, &d, pdMS_TO_TICKS(APP_DISPLAY_QUEUE_TIMEOUT_MS)) == pdPASS) {
            /* Chiếm SPI Mutex -> vẽ -> nhả Mutex */
            if (xSemaphoreTake(xMutexSPI, pdMS_TO_TICKS(APP_SPI_MUTEX_TIMEOUT_MS)) == pdTRUE) {
                prvUpdate(&d);
                xSemaphoreGive(xMutexSPI);
            }
        }

#if APP_DEBUG_BRINGUP
        bsp_led_toggle();                                   /* LED nháy: vòng lặp TaskDisplayUI đang chạy */
#endif
        vTaskDelay(pdMS_TO_TICKS(APP_DISPLAY_PERIOD_MS));   /* giải phóng CPU */
    }
}

BaseType_t task_display_ui_create(void)
{
    return xTaskCreate(prvDisplayTask, "DISPLAY", TASK_DISPLAY_STACK,
                       NULL, TASK_DISPLAY_PRIO, &xDisplayTaskHandle);
}
