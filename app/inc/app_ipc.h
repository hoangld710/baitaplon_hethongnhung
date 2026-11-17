/* app_ipc.h - Đối tượng IPC dùng chung giữa các Task */
#ifndef APP_IPC_H
#define APP_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "hal_ds1307.h"

/* Gói tin Queue 1: TaskSensors -> TaskProcessAlarm */
typedef struct {
    int32_t raw_net;        /* (raw đã lọc - offset tare), count */
    float   weight_g;       /* khối lượng, gram (có thể hơi âm quanh 0) */
} ScaleData_t;

/* Gói tin Queue 2: TaskProcessAlarm -> TaskDisplayUI */
typedef struct {
    float          weight_kg;       /* >= 0 */
    uint16_t       max_scale_g;     /* 2000 / 3000 / 5000 */
    bool           overload;        /* weight_g > max_scale_g */
    bool           starting;        /* true: vừa bật máy, HX711 đang ổn định/tare (chưa phải lỗi) */
    bool           sensor_ok;       /* false nếu quá hạn không nhận được dữ liệu HX711 */
    bool           rtc_ok;          /* false nếu đọc DS1307 thất bại */
    RtcDateTime_t  dt;
    int32_t        raw_net;         /* phục vụ hiệu chuẩn (APP_DEBUG_SHOW_RAW) */
} ScaleDisplay_t;

extern QueueHandle_t     xQueueSensor;    /* Queue 1 */
extern QueueHandle_t     xQueueDisplay;   /* Queue 2 */
extern SemaphoreHandle_t xMutexI2C;       /* bảo vệ bus I2C1 (DS1307) */
extern SemaphoreHandle_t xMutexSPI;       /* bảo vệ bus SPI1 (ST7789) */

/* Mức dùng stack đỉnh (High Water Mark, words còn trống) của từng task: xem trong debugger */
extern volatile UBaseType_t g_stackHwm[3];   /* [0]=Sensors [1]=ProcessAlarm [2]=DisplayUI */

BaseType_t app_ipc_init(void);

#endif /* APP_IPC_H */
