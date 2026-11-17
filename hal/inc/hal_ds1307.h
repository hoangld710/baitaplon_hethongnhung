#ifndef HAL_DS1307_H
#define HAL_DS1307_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t year;      /* 2000..2099 */
    uint8_t  month;     /* 1..12 */
    uint8_t  date;      /* 1..31 */
    uint8_t  dow;       /* 1..7 (1 = Chủ nhật) */
    uint8_t  hour;      /* 0..23 */
    uint8_t  min;       /* 0..59 */
    uint8_t  sec;       /* 0..59 */
} RtcDateTime_t;

/* Các hàm dưới đây KHÔNG tự khóa mutex: task gọi phải giữ xMutexI2C. */
bool hal_ds1307_read(RtcDateTime_t *t);
bool hal_ds1307_write(const RtcDateTime_t *t);
bool hal_ds1307_set_compile_time(void);     /* đặt giờ = thời điểm biên dịch (dùng 1 lần để set RTC) */

#endif
