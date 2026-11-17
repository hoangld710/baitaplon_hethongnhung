#include "bsp_delay.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

void bsp_delay_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

void bsp_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks) { /* wait */ }
}

/* Chỉ gọi từ trong Task (scheduler đã chạy). Không dùng HAL_Delay theo quy chuẩn kỷ luật mục 5.2. */
void bsp_delay_ms(uint32_t ms)
{
    TickType_t t = pdMS_TO_TICKS(ms);
    vTaskDelay((t == 0U) ? 1U : t);
}
