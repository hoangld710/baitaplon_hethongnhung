/* bsp_pins.h - Pin mapping cho STM32F103C8T6 (Blue Pill)
 *
 *  HX711   : DT=PB0  SCK=PB1
 *  Buttons : PB12 (2kg)  PB13 (3kg)  PB14 (5kg)   -> nối xuống GND, dùng pull-up nội
 *  DS1307  : I2C1  SCL=PB6  SDA=PB7
 *  ST7789  : SPI1  SCL=PA5  SDA(MOSI)=PA7 | DC=PA3  RES=PA2  (BLK -> 3V3)
 *            CS=PA4 chỉ dùng nếu module có chân CS (xem ST7789_USE_CS trong hal_st7789.h)
 */
#ifndef BSP_PINS_H
#define BSP_PINS_H

#include "stm32f1xx_hal.h"

/* HX711 */
#define HX711_DT_PORT    GPIOB
#define HX711_DT_PIN     GPIO_PIN_0
#define HX711_SCK_PORT   GPIOB
#define HX711_SCK_PIN    GPIO_PIN_1

/* 3 nút nhấn (active-low) */
#define BTN_2KG_PORT     GPIOB
#define BTN_2KG_PIN      GPIO_PIN_12
#define BTN_3KG_PORT     GPIOB
#define BTN_3KG_PIN      GPIO_PIN_13
#define BTN_5KG_PORT     GPIOB
#define BTN_5KG_PIN      GPIO_PIN_14

/* TFT ST7789 (chân điều khiển; SCL/SDA do SPI1 đảm nhiệm) */
#define TFT_CS_PORT      GPIOA
#define TFT_CS_PIN       GPIO_PIN_4
#define TFT_DC_PORT      GPIOA
#define TFT_DC_PIN       GPIO_PIN_3
#define TFT_RST_PORT     GPIOA
#define TFT_RST_PIN      GPIO_PIN_2

/* LED onboard Blue Pill (PC13, sáng khi mức thấp) - dùng để chẩn đoán */
#define LED_PORT         GPIOC
#define LED_PIN          GPIO_PIN_13

#endif /* BSP_PINS_H */
