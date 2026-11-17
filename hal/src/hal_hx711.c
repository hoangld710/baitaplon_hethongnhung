#include "hal_hx711.h"
#include "bsp_pins.h"
#include "bsp_delay.h"

void hal_hx711_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    g.Pin   = HX711_SCK_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HX711_SCK_PORT, &g);
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);   /* SCK thấp: HX711 hoạt động */

    g.Pin  = HX711_DT_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;       /* chưa nối HX711 => DT = 1 = "chưa sẵn sàng" (HX711 tự kéo DT xuống khi có dữ liệu) */
    HAL_GPIO_Init(HX711_DT_PORT, &g);
}

bool hal_hx711_is_ready(void)
{
    return (HAL_GPIO_ReadPin(HX711_DT_PORT, HX711_DT_PIN) == GPIO_PIN_RESET);
}

bool hal_hx711_read_raw(int32_t *raw)
{
    uint32_t v = 0;
    uint32_t primask;

    if (!hal_hx711_is_ready()) {
        return false;
    }

    /* 25 xung SCK ~ 50 us. SCK cao > 60 us sẽ đưa HX711 vào power-down,
     * nên chặn ngắt trong lúc đọc (rất ngắn, không ảnh hưởng RTOS). */
    primask = __get_PRIMASK();
    __disable_irq();

    for (uint8_t i = 0; i < 24; i++) {
        HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
        bsp_delay_us(1);
        v <<= 1;
        if (HAL_GPIO_ReadPin(HX711_DT_PORT, HX711_DT_PIN) == GPIO_PIN_SET) {
            v |= 1U;
        }
        HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
        bsp_delay_us(1);
    }

    /* Xung thứ 25: chọn kênh A, gain 128 cho lần đọc kế tiếp */
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
    bsp_delay_us(1);
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
    bsp_delay_us(1);

    __set_PRIMASK(primask);

    if (v & 0x800000UL) {           /* mở rộng dấu 24 -> 32 bit */
        v |= 0xFF000000UL;
    }
    *raw = (int32_t)v;
    return true;
}
