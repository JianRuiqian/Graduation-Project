#include <rtthread.h>
#include "board.h"

#ifdef RT_USING_SPI
#include "drv_spi.h"
#ifdef RT_USING_W25QXX
#include "spi_flash_w25qxx.h"
#endif

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

int rt_platform_init(void)
{
#ifdef RT_USING_SPI
    rt_hw_spi3_init();
#ifdef RT_USING_W25QXX
    w25qxx_init("flash0", "spi30");
#endif /* RT_USING_W25QXX */
#endif /* RT_USING_SPI */

    return 0;
}
INIT_BOARD_EXPORT(rt_platform_init);
