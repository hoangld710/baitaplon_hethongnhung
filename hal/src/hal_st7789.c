#include "hal_st7789.h"
#include "hal_font5x7.h"
#include "bsp_pins.h"
#include "bsp_delay.h"
#include "stm32f1xx_hal.h"

extern SPI_HandleTypeDef hspi1;   /* CubeMX: Transmit Only Master, 8-bit, CPOL High, CPHA 2 Edge (mode 3) */

#define SPI_TIMEOUT_MS   100U
#define MAX_SCALE        8U

/* Ghi nhận lỗi SPI đầu tiên (HAL_ERROR=1, HAL_BUSY=2, HAL_TIMEOUT=3) để chẩn đoán */
static volatile uint8_t s_spi_err = 0;
static void spi_tx(const uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef r = HAL_SPI_Transmit(&hspi1, (uint8_t *)buf, len, SPI_TIMEOUT_MS);
    if (r != HAL_OK && s_spi_err == 0U) { s_spi_err = (uint8_t)r; }
}
uint8_t st7789_spi_error(void) { return s_spi_err; }

/* ---- GPIO điều khiển ---- */
#if ST7789_USE_CS
static inline void cs_low(void)  { HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET); }
static inline void cs_high(void) { HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);   }
#else
static inline void cs_low(void)  { }
static inline void cs_high(void) { }
#endif
static inline void dc_cmd(void)  { HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_RESET); }
static inline void dc_data(void) { HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);   }

static void write_cmd(uint8_t cmd)
{
    dc_cmd();
    cs_low();
    spi_tx(&cmd, 1);
    cs_high();
}

static void write_data(const uint8_t *data, uint16_t len)
{
    dc_data();
    cs_low();
    spi_tx(data, len);
    cs_high();
}

static void cmd_args(uint8_t cmd, const uint8_t *args, uint16_t n)
{
    write_cmd(cmd);
    write_data(args, n);
}

#define SEND(cmd, ...) do { static const uint8_t a_[] = { __VA_ARGS__ }; cmd_args((cmd), a_, sizeof a_); } while (0)

/* ---- Cửa sổ vẽ + truyền điểm ảnh ---- */
static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t a[4];
    uint16_t xs = (uint16_t)(x0 + ST7789_XSTART), xe = (uint16_t)(x1 + ST7789_XSTART);
    uint16_t ys = (uint16_t)(y0 + ST7789_YSTART), ye = (uint16_t)(y1 + ST7789_YSTART);

    a[0] = (uint8_t)(xs >> 8); a[1] = (uint8_t)xs; a[2] = (uint8_t)(xe >> 8); a[3] = (uint8_t)xe;
    cmd_args(0x2A, a, 4);                                   /* CASET */
    a[0] = (uint8_t)(ys >> 8); a[1] = (uint8_t)ys; a[2] = (uint8_t)(ye >> 8); a[3] = (uint8_t)ye;
    cmd_args(0x2B, a, 4);                                   /* RASET */
    write_cmd(0x2C);                                        /* RAMWR */
}

static void pix_begin(void) { dc_data(); cs_low(); }
static void pix_send(const uint8_t *buf, uint16_t len) { spi_tx(buf, len); }
static void pix_end(void)   { cs_high(); }

/* ---- Thử nhanh cấu hình SPI mà không cần CubeMX: thêm vào Keil > Options > C/C++ > Define, ví dụ
 *      ST7789_SPI_MODE=0                      (0 hoặc 3)
 *      ST7789_SPI_PRESCALER=SPI_BAUDRATEPRESCALER_16
 */
#if defined(ST7789_SPI_MODE) || defined(ST7789_SPI_PRESCALER)
static void spi_override(void)
{
#ifdef ST7789_SPI_MODE
    hspi1.Init.CLKPolarity = (ST7789_SPI_MODE == 3) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase    = (ST7789_SPI_MODE == 3) ? SPI_PHASE_2EDGE   : SPI_PHASE_1EDGE;
#endif
#ifdef ST7789_SPI_PRESCALER
    hspi1.Init.BaudRatePrescaler = ST7789_SPI_PRESCALER;
#endif
    (void)HAL_SPI_DeInit(&hspi1);
    (void)HAL_SPI_Init(&hspi1);
}
#endif

/* ---- API ---- */
bool st7789_init(void)
{
    GPIO_InitTypeDef g = {0};

#if defined(ST7789_SPI_MODE) || defined(ST7789_SPI_PRESCALER)
    spi_override();
#endif

    __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
#if ST7789_USE_CS
    g.Pin = TFT_CS_PIN;  HAL_GPIO_Init(TFT_CS_PORT,  &g);
#endif
    g.Pin = TFT_DC_PIN;  HAL_GPIO_Init(TFT_DC_PORT,  &g);
    g.Pin = TFT_RST_PIN; HAL_GPIO_Init(TFT_RST_PORT, &g);
    cs_high();

    /* Hard reset */
    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_SET);   bsp_delay_ms(10);
    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_RESET); bsp_delay_ms(10);
    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_SET);   bsp_delay_ms(120);

    write_cmd(0x01);  bsp_delay_ms(150);                    /* SWRESET */
    write_cmd(0x11);  bsp_delay_ms(120);                    /* SLPOUT  */

    SEND(0x3A, 0x55);                                       /* COLMOD: RGB565 16-bit */
    SEND(0x36, ST7789_MADCTL_VAL);                          /* MADCTL  */
    SEND(0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33);               /* PORCTRL */
    SEND(0xB7, 0x35);                                       /* GCTRL   */
    SEND(0xBB, 0x19);                                       /* VCOMS   */
    SEND(0xC0, 0x2C);                                       /* LCMCTRL */
    SEND(0xC2, 0x01);                                       /* VDVVRHEN */
    SEND(0xC3, 0x12);                                       /* VRHS    */
    SEND(0xC4, 0x20);                                       /* VDVS    */
    SEND(0xC6, 0x0F);                                       /* FRCTRL2: 60 Hz */
    SEND(0xD0, 0xA4, 0xA1);                                 /* PWCTRL1 */
    SEND(0xE0, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F,
               0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23);   /* PVGAMCTRL */
    SEND(0xE1, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F,
               0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23);   /* NVGAMCTRL */
    write_cmd(0x21);                                        /* INVON: panel IPS cần đảo màu */
    write_cmd(0x13);  bsp_delay_ms(10);                     /* NORON   */
    write_cmd(0x29);  bsp_delay_ms(120);                    /* DISPON  */

    st7789_fill_screen(ST7789_BLACK);
    return true;
}

void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint8_t  chunk[128];                    /* 64 điểm ảnh mỗi lần gửi, tiết kiệm stack */
    uint32_t total;

    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT || w == 0U || h == 0U) { return; }
    if ((uint32_t)x + w > ST7789_WIDTH)  { w = (uint16_t)(ST7789_WIDTH  - x); }
    if ((uint32_t)y + h > ST7789_HEIGHT) { h = (uint16_t)(ST7789_HEIGHT - y); }

    for (uint16_t i = 0; i < 64U; i++) {
        chunk[2U * i]      = (uint8_t)(color >> 8);
        chunk[2U * i + 1U] = (uint8_t)(color & 0xFFU);
    }

    set_window(x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U));
    total = (uint32_t)w * h;
    pix_begin();
    while (total > 0U) {
        uint16_t n = (total > 64U) ? 64U : (uint16_t)total;
        pix_send(chunk, (uint16_t)(2U * n));
        total -= n;
    }
    pix_end();
}

void st7789_fill_screen(uint16_t color)
{
    st7789_fill_rect(0, 0, ST7789_WIDTH, ST7789_HEIGHT, color);
}

static void draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t s)
{
    uint8_t line[6U * MAX_SCALE * 2U];
    const uint16_t w = (uint16_t)(6U * s), h = (uint16_t)(8U * s);
    const uint8_t *glyph;

    if (c >= 'a' && c <= 'z') { c = (char)(c - 32); }
    if (c < FONT5X7_FIRST || c > FONT5X7_LAST) { c = '?'; }
    glyph = &font5x7[((uint8_t)c - FONT5X7_FIRST) * 5U];

    if ((uint32_t)x + w > ST7789_WIDTH || (uint32_t)y + h > ST7789_HEIGHT) { return; }

    set_window(x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U));
    pix_begin();
    for (uint8_t row = 0; row < 8U; row++) {
        uint16_t n = 0;
        for (uint8_t col = 0; col < 6U; col++) {            /* cột 5 = khoảng trống giữa các ký tự */
            uint16_t color = ((col < 5U) && ((glyph[col] >> row) & 1U)) ? fg : bg;
            for (uint8_t sx = 0; sx < s; sx++) {
                line[n++] = (uint8_t)(color >> 8);
                line[n++] = (uint8_t)(color & 0xFFU);
            }
        }
        for (uint8_t sy = 0; sy < s; sy++) {
            pix_send(line, n);
        }
    }
    pix_end();
}

void st7789_draw_string(uint16_t x, uint16_t y, const char *str,
                        uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (scale == 0U || scale > MAX_SCALE) { scale = 1U; }

    while (*str != '\0') {
        draw_char(x, y, *str, fg, bg, scale);
        x = (uint16_t)(x + 6U * scale);
        str++;
    }
}
