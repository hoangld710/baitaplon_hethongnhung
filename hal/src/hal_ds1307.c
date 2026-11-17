#include "hal_ds1307.h"
#include "stm32f1xx_hal.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;         /* do CubeMX sinh ra */

#define DS1307_ADDR         (0x68U << 1)
#define DS1307_REG_TIME     0x00U
#define DS1307_TIMEOUT_MS   20U

static uint8_t bcd2dec(uint8_t v) { return (uint8_t)(((v >> 4) * 10U) + (v & 0x0FU)); }
static uint8_t dec2bcd(uint8_t v) { return (uint8_t)(((v / 10U) << 4) | (v % 10U)); }

bool hal_ds1307_read(RtcDateTime_t *t)
{
    uint8_t b[7];

    if (HAL_I2C_Mem_Read(&hi2c1, DS1307_ADDR, DS1307_REG_TIME, I2C_MEMADD_SIZE_8BIT,
                         b, sizeof b, DS1307_TIMEOUT_MS) != HAL_OK) {
        return false;
    }

    /* Bit7 của thanh ghi giây = CH (Clock Halt): =1 nghĩa là dao động đang dừng (chưa set giờ / mất pin) */
    if (b[0] & 0x80U) {
        return false;
    }

    t->sec   = bcd2dec(b[0] & 0x7FU);
    t->min   = bcd2dec(b[1] & 0x7FU);
    if (b[2] & 0x40U) {                     /* chế độ 12h: bit5 = PM, bit4..0 = giờ 1..12 */
        uint8_t h = bcd2dec(b[2] & 0x1FU) % 12U;
        t->hour = (b[2] & 0x20U) ? (uint8_t)(h + 12U) : h;
    } else {
        t->hour = bcd2dec(b[2] & 0x3FU);    /* chế độ 24h */
    }
    t->dow   = bcd2dec(b[3] & 0x07U);
    t->date  = bcd2dec(b[4] & 0x3FU);
    t->month = bcd2dec(b[5] & 0x1FU);
    t->year  = (uint16_t)(2000U + bcd2dec(b[6]));

    /* Kiểm tra hợp lệ: tránh hiển thị rác khi RTC chưa được set hoặc bus lỗi */
    if (t->month < 1U || t->month > 12U || t->date < 1U || t->date > 31U ||
        t->hour > 23U || t->min > 59U || t->sec > 59U) {
        return false;
    }
    return true;
}

bool hal_ds1307_write(const RtcDateTime_t *t)
{
    uint8_t b[7];

    b[0] = dec2bcd(t->sec);                /* bit7 = 0: xóa CH, khởi động dao động */
    b[1] = dec2bcd(t->min);
    b[2] = dec2bcd(t->hour);                /* bit6 = 0: 24h */
    b[3] = dec2bcd(t->dow);
    b[4] = dec2bcd(t->date);
    b[5] = dec2bcd(t->month);
    b[6] = dec2bcd((uint8_t)(t->year - 2000U));

    return (HAL_I2C_Mem_Write(&hi2c1, DS1307_ADDR, DS1307_REG_TIME, I2C_MEMADD_SIZE_8BIT,
                              b, sizeof b, DS1307_TIMEOUT_MS) == HAL_OK);
}

bool hal_ds1307_set_compile_time(void)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    static const uint8_t mt[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    const char d[] = __DATE__;      /* "Nov 12 2026" */
    const char s[] = __TIME__;      /* "09:41:05"    */
    RtcDateTime_t r;
    uint8_t m;
    int y;

    for (m = 0; m < 12U; m++) {
        if (strncmp(d, &months[m * 3U], 3) == 0) { break; }
    }
    if (m == 12U) { return false; }

    r.month = (uint8_t)(m + 1U);
    r.date  = (uint8_t)(((d[4] == ' ') ? 0 : (d[4] - '0')) * 10 + (d[5] - '0'));
    r.year  = (uint16_t)((d[7] - '0') * 1000 + (d[8] - '0') * 100 + (d[9] - '0') * 10 + (d[10] - '0'));
    r.hour  = (uint8_t)((s[0] - '0') * 10 + (s[1] - '0'));
    r.min   = (uint8_t)((s[3] - '0') * 10 + (s[4] - '0'));
    r.sec   = (uint8_t)((s[6] - '0') * 10 + (s[7] - '0'));

    /* Thứ trong tuần (Sakamoto): 0 = Chủ nhật */
    y = (int)r.year - ((r.month < 3U) ? 1 : 0);
    r.dow = (uint8_t)(((y + y / 4 - y / 100 + y / 400 + mt[r.month - 1U] + r.date) % 7) + 1);

    return hal_ds1307_write(&r);
}
