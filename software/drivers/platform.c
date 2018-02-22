#include <rtthread.h>
#include "board.h"

#ifdef RT_USING_SDIO
#include <drivers/mmcsd_core.h>
#include "sdmmc.h"
#endif

#ifdef RT_USING_SPI
#include "spibus.h"
#include "spi_flash_w25qxx.h"

/*
SPI3_MOSI: PB5
SPI3_MISO: PB4
SPI3_SCK : PB3

CS30: PA15 SPI FLASH
*/
static void rt_hw_spi3_init(void)
{
    /* register spi bus */
    {
        static struct stm32_spi_bus stm32_spi;
        GPIO_InitTypeDef GPIO_InitStructure;

        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

        /*!< SPI SCK pin configuration */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        /* Connect alternate function */
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI3);
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI3);
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI3);

        stm32_spi_register(SPI3, &stm32_spi, "spi3");
    }

    /* attach cs */
    {
        static struct rt_spi_device spi_device;
        static struct stm32_spi_cs  spi_cs;

        GPIO_InitTypeDef GPIO_InitStructure;

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

        /* spi30: PA15 */
        spi_cs.GPIOx = GPIOA;
        spi_cs.GPIO_Pin = GPIO_Pin_15;
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

        GPIO_InitStructure.GPIO_Pin = spi_cs.GPIO_Pin;
        GPIO_SetBits(spi_cs.GPIOx, spi_cs.GPIO_Pin);
        GPIO_Init(spi_cs.GPIOx, &GPIO_InitStructure);

        rt_spi_bus_attach_device(&spi_device, "spi30", "spi3", (void *)&spi_cs);
    }
}
#endif

#ifdef PKG_USING_MARVELLWIFI
static void rt_hw_wifi_reset(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    /* PWDN: PD3 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* hardware reset wifi card */
    GPIO_ResetBits(GPIOD, GPIO_Pin_3);
    rt_hw_us_delay(1000);
    GPIO_SetBits(GPIOD, GPIO_Pin_3);
}
#endif  /* PKG_USING_MARVELLWIFI */

void rt_platform_init(void)
{
#ifdef RT_USING_SPI
    rt_hw_spi3_init();

#ifdef RT_USING_DFS
    w25qxx_init("flash0", "spi30");
#endif /* RT_USING_DFS */
#endif /* RT_USING_SPI */

#ifdef PKG_USING_MARVELLWIFI
    rt_hw_wifi_reset();
#endif  /* PKG_USING_MARVELLWIFI */

#ifdef RT_USING_SDIO
    rt_mmcsd_core_init();
    stm32f4xx_sdio_init();
    mmcsd_delay_ms(200);
#endif  /* RT_USING_SDIO */
}
