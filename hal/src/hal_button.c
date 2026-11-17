#include "hal_button.h"
#include "bsp_pins.h"

void hal_button_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    g.Pin  = BTN_2KG_PIN | BTN_3KG_PIN | BTN_5KG_PIN;   /* cùng GPIOB */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &g);
}

uint8_t hal_button_read(void)
{
    uint8_t m = 0;

    if (HAL_GPIO_ReadPin(BTN_2KG_PORT, BTN_2KG_PIN) == GPIO_PIN_RESET) { m |= HAL_BTN_2KG; }
    if (HAL_GPIO_ReadPin(BTN_3KG_PORT, BTN_3KG_PIN) == GPIO_PIN_RESET) { m |= HAL_BTN_3KG; }
    if (HAL_GPIO_ReadPin(BTN_5KG_PORT, BTN_5KG_PIN) == GPIO_PIN_RESET) { m |= HAL_BTN_5KG; }
    return m;
}
