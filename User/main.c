#include "main.h"
#include "SEGGER_RTT.h"
#include "lcd_st7586.h"
#include "lcd_test.h"

/* delay functions using DWT cycle counter (no interrupts, won't corrupt SPI) */
extern uint32_t SystemCoreClock;

static uint8_t _dwt_ready = 0;

static void delay_init(void)
{
    if (_dwt_ready) return;
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    _dwt_ready = 1;
}

void delay_ms(uint32_t ms)
{
    delay_init();
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = ms * (SystemCoreClock / 1000U);
    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
        __NOP();
    }
}

void lcd_gpio_init(void)
{
    /* enable GPIOA and GPIOC clocks */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);

    /* PA5 - SPI0_SCK (AF5 push-pull) */
		gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_5);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
    

    /* PA7 - SPI0_MOSI (AF5 push-pull) */
		gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    

    /* PA4 - LCD_CS (GPIO output, push-pull) */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

    /* PA3 - LCD_BL (GPIO output, push-pull) */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);

    /* PA2 - 备用控制引脚 (GPIO output, push-pull) */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
		

    /* PC4 - LCD_DC (GPIO output, push-pull) */
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

    /* PC5 - LCD_RST (GPIO output, push-pull) */
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);

    /* set initial states */
    LCD_CS_HIGH();     /* CS inactive */
    LCD_DC_LOW();      /* command mode */
    LCD_RST_HIGH();    /* release reset */
    LCD_BL_OFF();      /* backlight off */
    gpio_bit_set(GPIOA, GPIO_PIN_2);  /* PA2 low */
}

void lcd_spi_init(void)
{
    spi_parameter_struct spi_param;

    /* enable SPI0 clock */
    rcu_periph_clock_enable(RCU_SPI0);

    /* initialize SPI parameters with default values */
    spi_struct_para_init(&spi_param);

    /* configure SPI0 as master */
    spi_param.device_mode          = SPI_MASTER;
    spi_param.trans_mode           = SPI_TRANSMODE_BDTRANSMIT;
    spi_param.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_param.nss                  = SPI_NSS_SOFT;
    spi_param.endian               = SPI_ENDIAN_MSB;
    spi_param.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;  /* CPOL=0, CPHA=0 */
    spi_param.prescale             = SPI_PSC_4;                /* APB2/4 */

    spi_init(SPI0, &spi_param);

    /* enable SPI0 */
    spi_enable(SPI0);
}

int main(void)
{
#ifdef USE_RTT
    SEGGER_RTT_Init();
#endif

    /* initialize LCD GPIO and SPI */
    lcd_gpio_init();
    lcd_spi_init();

    /* hardware reset sequence for LCD */
    LCD_RST_LOW();
    for (volatile uint32_t i = 0; i < 10000; i++);
    LCD_RST_HIGH();
    for (volatile uint32_t i = 0; i < 10000; i++);

    /* enable backlight */
//    LCD_BL_ON();

    /* initialize LCD */
    ST7586_initialize();

    LCD_Test_Loop();
}
