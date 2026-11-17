#include "bsp_led.h"
#include "bsp_pins.h"
#include "bsp_delay.h"

void bsp_led_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    g.Pin   = LED_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &g);
    bsp_led_off();
}

void bsp_led_on(void)     { HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); }
void bsp_led_off(void)    { HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);   }
void bsp_led_toggle(void) { HAL_GPIO_TogglePin(LED_PORT, LED_PIN); }

static void wait_ms(uint32_t ms)
{
    while (ms--) { bsp_delay_us(1000); }
}

void bsp_led_fatal(uint8_t code)
{
    for (;;) {
        for (uint8_t i = 0; i < code; i++) {
            bsp_led_on();  wait_ms(200);
            bsp_led_off(); wait_ms(200);
        }
        wait_ms(1500);
    }
}
