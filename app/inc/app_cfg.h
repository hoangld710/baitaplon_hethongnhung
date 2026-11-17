/* app_cfg.h - Bảng thông số RTOS & ứng dụng (Leader chốt, không tự ý sửa trong nhánh feature) */
#ifndef APP_CFG_H
#define APP_CFG_H

#include "FreeRTOS.h"
#include "task.h"

/* ---- Priority: Sensors (High) > ProcessAlarm (Med) > DisplayUI (Low) ---- */
#define TASK_SENSORS_PRIO        (tskIDLE_PRIORITY + 3)
#define TASK_PROCESS_PRIO        (tskIDLE_PRIORITY + 2)
#define TASK_DISPLAY_PRIO        (tskIDLE_PRIORITY + 1)

/* ---- Stack (đơn vị: word = 4 byte). Kiểm tra lại bằng g_stackHwm[] ---- */
#define TASK_SENSORS_STACK       256U
#define TASK_PROCESS_STACK       256U
#define TASK_DISPLAY_STACK       640U

/* ---- Queue ---- */
#define APP_Q_SENSOR_LEN         4U      /* Queue 1: Sensors -> ProcessAlarm */
#define APP_Q_DISPLAY_LEN        1U      /* Queue 2: ProcessAlarm -> DisplayUI (mailbox, ghi đè bản cũ) */
#if (APP_Q_DISPLAY_LEN != 1U)
#error "Queue 2 dùng xQueueOverwrite() nên bắt buộc APP_Q_DISPLAY_LEN = 1"
#endif

/* ---- Timeout (ms): KHÔNG dùng portMAX_DELAY ---- */
#define APP_I2C_MUTEX_TIMEOUT_MS     20U
#define APP_SPI_MUTEX_TIMEOUT_MS     50U
#define APP_PROCESS_SENSOR_TIMEOUT_MS 400U   /* quá thời gian này không có dữ liệu = lỗi cảm biến */
#define APP_DISPLAY_QUEUE_TIMEOUT_MS 500U
#define APP_DISPLAY_PERIOD_MS        100U    /* vTaskDelay cuối vòng của TaskDisplayUI */
#define APP_BOOT_GRACE_MS            5000U   /* sau khi bật máy, trong thời gian này chưa có dữ liệu = "STARTING" (đang tare), không phải lỗi */

/* ---- Dải đo (gram) ---- */
#define APP_SCALE_2KG_G          2000U
#define APP_SCALE_3KG_G          3000U
#define APP_SCALE_5KG_G          5000U
#define APP_SCALE_DEFAULT_G      APP_SCALE_2KG_G

/* ---- HX711 / lọc ---- */
/* Hệ số hiệu chuẩn: số count trên 1 gram.
 * Load cell YZC-131 5kg: ~1.0 mV/V, HX711 gain 128 => lý thuyết ~430 count/g (sai số +-10..15% theo từng cell).
 * VẪN PHẢI hiệu chuẩn lại bằng vật đã biết khối lượng (xem README mục 4).
 * Cho phép số âm nếu cân hiển thị ngược dấu khi đặt vật lên (hoặc đổi dây xanh/trắng). */
#define APP_HX711_COUNTS_PER_GRAM    430.0f
#define APP_MAVG_WINDOW              8U      /* cửa sổ lọc trung bình động */
#define APP_TARE_SAMPLES             16U     /* số mẫu lấy trung bình để tare lúc khởi động */
#define APP_HX711_SETTLE_MS          500U    /* chờ HX711 ổn định sau khi cấp điện */
#define APP_HX711_READY_TIMEOUT_MS   250U    /* tối đa chờ DT xuống thấp (10 SPS = 100 ms/mẫu) */
#define APP_SENSOR_MIN_GAP_MS        20U     /* nghỉ tối thiểu sau mỗi lần đọc: TaskSensors (ưu tiên cao nhất) KHÔNG BAO GIỜ được chạy không nghỉ */

/* ---- RTC ---- */
/* 1: set DS1307 = giờ lúc biên dịch. Nạp 1 lần với giá trị 1, rồi đổi về 0 và nạp lại! */
#ifndef APP_RTC_SET_FROM_COMPILE_TIME
#define APP_RTC_SET_FROM_COMPILE_TIME 0
#endif

/* ---- Debug ---- */
#ifndef APP_DEBUG_SHOW_RAW
#define APP_DEBUG_SHOW_RAW       1       /* 1: hiện giá trị RAW trên TFT để hiệu chuẩn; đặt 0 khi nộp bài */
#endif

/* 1: chế độ chẩn đoán lần đầu nạp: LED PC13 sáng khi task màn hình bắt đầu, nháy khi vòng lặp chạy,
 *    và màn hình lần lượt đổ ĐỎ, XANH LÁ, XANH DƯƠNG, TRẮNG trước khi vào giao diện. Đặt 0 khi nộp bài. */
#ifndef APP_DEBUG_BRINGUP
#define APP_DEBUG_BRINGUP        1
#endif

#endif /* APP_CFG_H */
