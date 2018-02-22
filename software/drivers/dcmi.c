#include <rtthread.h>
#include <board.h>
#include "dcmi.h"

//#define DCMI_DMA_STREAM1
#define DCMI_DMA_STREAM7

#ifdef DCMI_DMA_STREAM1
#define DCMI_DMA_STREAM             DMA2_Stream1
#define DCMI_DMA_CHANNEL            DMA_Channel_1
#define DCMI_DMA_FLAG_TCIF          DMA_FLAG_TCIF1
#define DCMI_DMA_IT_TCIF            DMA_IT_TCIF1
#define DCMI_DMA_IRQn               DMA2_Stream1_IRQn
#define DCMI_DMA_IRQHANDLER         DMA2_Stream1_IRQHandler
#elif defined DCMI_DMA_STREAM7
#define DCMI_DMA_STREAM             DMA2_Stream7
#define DCMI_DMA_CHANNEL            DMA_Channel_1
#define DCMI_DMA_FLAG_TCIF          DMA_FLAG_TCIF7
#define DCMI_DMA_IT_TCIF            DMA_IT_TCIF7
#define DCMI_DMA_IRQn               DMA2_Stream7_IRQn
#define DCMI_DMA_IRQHANDLER         DMA2_Stream7_IRQHandler
#endif

static struct
{
    uint32_t cnt;
    uint32_t num;
    uint32_t siz;
    uint32_t buf;
} hdcmi;

/* -----------------------------Static Functions----------------------------- */
static void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
                           RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    /* HSYNC(PA4), PCLK(PA6) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* D5..D6(PB6/8), VSYNC(PB7) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* D0..D1(PC6/7) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* D2..D4(PE0/1/4), D7(PE6) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_6;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    /* Connect DCMI pins to AF13 */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_DCMI); //PA4, AF13 DCMI_HSYNC
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_DCMI); //PA6, AF13 DCMI_PCLK
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_DCMI); //PB7, AF13 DCMI_VSYNC
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_DCMI); //PC6, AF13 DCMI_D0
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_DCMI); //PC7, AF13 DCMI_D1
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource0, GPIO_AF_DCMI); //PE0, AF13 DCMI_D2
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource1, GPIO_AF_DCMI); //PE1, AF13 DCMI_D3
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource4, GPIO_AF_DCMI); //PE4, AF13 DCMI_D4
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_DCMI); //PB6, AF13 DCMI_D5
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_DCMI); //PB8, AF13 DCMI_D6
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource6, GPIO_AF_DCMI); //PE6, AF13 DCMI_D7
}

static void DCMI_Configuration(void)
{
    DCMI_InitTypeDef DCMI_InitStructure;

    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);

    DCMI_DeInit();

    /* DCMI configuration */
    DCMI_InitStructure.DCMI_CaptureMode = DCMI_CaptureMode_Continuous;
    DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
    DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;
    DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_Low;
    DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_Low;
    DCMI_InitStructure.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;
    DCMI_InitStructure.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b;
    DCMI_Init(&DCMI_InitStructure);

    DCMI_Cmd(ENABLE);

    DCMI_ITConfig(DCMI_IT_FRAME, ENABLE);
}

static void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* dcmi interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = DCMI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* dma interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = DCMI_DMA_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/* -----------------------------Global Functions----------------------------- */
RT_WEAK void dcmi_interrupt_frame_callback(void)
{
    /* This function Should not be modified, when the callback is needed,
       the dcmi_interrupt_frame_callback could be implemented in the user file
    */
}

void DCMI_ProcessIRQSrc(void)
{
    if (DCMI_GetITStatus(DCMI_IT_FRAME) == SET)
    {
        /* Clear DCMI frame interrupt pending bit */
        DCMI_ClearITPendingBit(DCMI_IT_FRAME);

        dcmi_interrupt_frame_callback();
    }
}

/*******************************************************************************
* Function Name  : DCMI_IRQHandler
* Description    : This function handles DCMI global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DCMI_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    /* Process All DCMI Interrupt Sources */
    DCMI_ProcessIRQSrc();

    /* leave interrupt */
    rt_interrupt_leave();
}

void DCMI_DMA_IRQHANDLER(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    if (DMA_GetITStatus(DCMI_DMA_STREAM, DCMI_DMA_IT_TCIF))
    {
        /* Clear DMA Stream Transfer Complete interrupt pending bit */
        DMA_ClearITPendingBit(DCMI_DMA_STREAM, DCMI_DMA_IT_TCIF);

        if (hdcmi.cnt != 0)
        {
            uint32_t addr;

            /* Update memory 0 address location */
            if (DMA_GetCurrentMemoryTarget(DCMI_DMA_STREAM) != 0 &&
                    (hdcmi.cnt % 2 == 0))
            {
                addr = DCMI_DMA_STREAM->M0AR;
                DMA_MemoryTargetConfig(DCMI_DMA_STREAM,
                                       addr + (8 * hdcmi.siz), DMA_Memory_0);
                hdcmi.cnt--;
            }
            /* Update memory 1 address location */
            else if (DMA_GetCurrentMemoryTarget(DCMI_DMA_STREAM) == 0 &&
                     (hdcmi.cnt % 2 == 1))
            {
                addr = DCMI_DMA_STREAM->M1AR;
                DMA_MemoryTargetConfig(DCMI_DMA_STREAM,
                                       addr + (8 * hdcmi.siz), DMA_Memory_1);
                hdcmi.cnt--;
            }
        }
        /* Update memory 0 address location */
        else if (DMA_GetCurrentMemoryTarget(DCMI_DMA_STREAM) != 0)
        {
            DMA_MemoryTargetConfig(DCMI_DMA_STREAM,
                                   hdcmi.buf, DMA_Memory_0);
        }
        /* Update memory 1 address location */
        else if (DMA_GetCurrentMemoryTarget(DCMI_DMA_STREAM) == 0)
        {
            DMA_MemoryTargetConfig(DCMI_DMA_STREAM,
                                   hdcmi.buf + (4 * hdcmi.siz), DMA_Memory_1);
            hdcmi.cnt = hdcmi.num;
        }
    }

    /* leave interrupt */
    rt_interrupt_leave();
}

void dcmi_xfer_start(void)
{
    DMA_Cmd(DCMI_DMA_STREAM, ENABLE);
    DCMI_CaptureCmd(ENABLE);
}

void dcmi_xfer_stop(void)
{
    DCMI_CaptureCmd(DISABLE);
    while (DCMI->CR & DCMI_CR_CAPTURE);
    DMA_Cmd(DCMI_DMA_STREAM, DISABLE);
}

void dcmi_xfer_resume(void)
{
    DCMI_CaptureCmd(ENABLE);
}

void dcmi_xfer_suspend(void)
{
    DCMI_CaptureCmd(DISABLE);
}

void dcmi_xfer_config(void *buf, unsigned long len)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    DMA_DeInit(DCMI_DMA_STREAM);
    while (DMA_GetCmdStatus(DCMI_DMA_STREAM) != DISABLE);

    /* bytes to words */
    hdcmi.siz = len / 4;
    /* dma double buffer mode */
    if (hdcmi.siz > 0xffff)
    {
        hdcmi.cnt = 1;
        while (hdcmi.siz > 0xffff)
        {
            hdcmi.siz /= 2;
            hdcmi.cnt *= 2;
        }
        hdcmi.cnt -= 2;
        hdcmi.num = hdcmi.cnt;
        hdcmi.buf = (uint32_t)buf;
        DMA_DoubleBufferModeConfig(DCMI_DMA_STREAM,
                                   hdcmi.buf + (4 * hdcmi.siz), DMA_Memory_0);
        DMA_DoubleBufferModeCmd(DCMI_DMA_STREAM, ENABLE);
        DMA_ITConfig(DCMI_DMA_STREAM, DMA_IT_TC, ENABLE);
    }

    DMA_InitStructure.DMA_BufferSize = hdcmi.siz ? hdcmi.siz : 1;
    DMA_InitStructure.DMA_Channel = DCMI_DMA_CHANNEL;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;

    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buf;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (DCMI->DR);
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;

    DMA_Init(DCMI_DMA_STREAM, &DMA_InitStructure);
}

unsigned short dcmi_xfer_remaining(void)
{
    return DMA_GetCurrDataCounter(DCMI_DMA_STREAM) * 4;
}

int stm32_dcmi_init(void)
{
    GPIO_Configuration();
    DCMI_Configuration();
    NVIC_Configuration();

    return RT_EOK;
}
INIT_BOARD_EXPORT(stm32_dcmi_init);
