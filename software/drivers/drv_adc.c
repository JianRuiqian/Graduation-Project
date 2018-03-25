#include <rtthread.h>
#include <board.h>
#include "drv_adc.h"

#define ADCx                        ADC1
#define ADCx_RCC                    RCC_APB2Periph_ADC1

#ifdef ADC_USE_DMA
#define ADCx_DMA_RCC                RCC_AHB1Periph_DMA2
#define ADCx_DMA_CHANNEL            DMA_Channel_0
#define ADCx_DMA_STREAM             DMA2_Stream4
#define ADCx_DMA_STREAM_IRQn        DMA2_Stream4_IRQn
#define ADCx_DMA_STREAM_IRQHandler  DMA2_Stream4_IRQHandler
#define ADCx_DMA_FLAG_TC            DMA_FLAG_TCIF4
#define ADCx_DMA_IT_TC              DMA_IT_TCIF4
#endif

/* -----------------------------Static Functions----------------------------- */
static void RCC_Configuration(void)
{
    RCC_APB2PeriphClockCmd(ADCx_RCC, ENABLE);
#ifdef ADC_USE_DMA
    RCC_AHB1PeriphClockCmd(ADCx_DMA_RCC, ENABLE);
#endif
}

static void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    /* ADC Channel 10 -> PC0
       ADC Channel 11 -> PC1
       ADC Channel 12 -> PC2
       ADC Channel 13 -> PC3
    */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static void ADC_Configuration(uint8_t NbrOfConversion)
{
    ADC_InitTypeDef ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;

    RT_ASSERT(NbrOfConversion <= 6);

    /* ADC Common Init */
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC Init */
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = NbrOfConversion;
    ADC_Init(ADCx, &ADC_InitStructure);

    /* ADCx regular channels 10~13 configuration */
    ADC_RegularChannelConfig(ADCx, ADC_Channel_10, 1, ADC_SampleTime_480Cycles);
    ADC_RegularChannelConfig(ADCx, ADC_Channel_11, 2, ADC_SampleTime_480Cycles);
    ADC_RegularChannelConfig(ADCx, ADC_Channel_12, 3, ADC_SampleTime_480Cycles);
    ADC_RegularChannelConfig(ADCx, ADC_Channel_13, 4, ADC_SampleTime_480Cycles);

    if (NbrOfConversion >= 5)
    {
        ADC_RegularChannelConfig(ADCx, ADC_Channel_Vbat, 5, ADC_SampleTime_480Cycles);
        ADC_VBATCmd(ENABLE);
    }

    /* Enable ADCx */
    ADC_Cmd(ADCx, ENABLE);
}

#ifdef ADC_USE_DMA
static void DMA_Configuration(uint16_t *BufferSRC, uint32_t BufferSize)
{
    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(ADCx_DMA_STREAM);
    while (DMA_GetCmdStatus(ADCx_DMA_STREAM) != DISABLE);

    /* ADCx_DMA_STREAM ADCx_DMA_CHANNEL configuration */
    DMA_InitStructure.DMA_Channel = ADCx_DMA_CHANNEL;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADCx->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)BufferSRC;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = BufferSize;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(ADCx_DMA_STREAM, &DMA_InitStructure);
    
    DMA_ITConfig(ADCx_DMA_STREAM, DMA_IT_TC, ENABLE);

    DMA_Cmd(ADCx_DMA_STREAM, ENABLE);
}
#endif

static void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

#ifdef ADC_USE_DMA
    /* DMA interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = ADCx_DMA_STREAM_IRQn;
#else
    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
#endif
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/* -----------------------------Global Functions----------------------------- */
RT_WEAK void adc_xferdone_callback(void)
{
    /* This function Should not be modified, when the callback is needed, the 
       adc_xferdone_callback could be implemented in the user file */
}

#ifdef ADC_USE_DMA
void ADCx_DMA_STREAM_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();
    
    if (DMA_GetITStatus(ADCx_DMA_STREAM, ADCx_DMA_IT_TC) != RESET)
    {
        /* Clear DMA transfer complete interrupt pending bit */
        DMA_ClearITPendingBit(ADCx_DMA_STREAM, ADCx_DMA_IT_TC);

        adc_xferdone_callback();
    }
    
    /* leave interrupt */
    rt_interrupt_leave();
}
#else
static unsigned short *_value;
static unsigned char _channels;

void ADC_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();
    
    if (ADC_GetITStatus(ADCx, ADC_IT_EOC) != RESET)
    {
        static uint8_t channel = 0;
        
        /* Clear ADCx conversion complete interrupt pending bit */
        ADC_ClearITPendingBit(ADCx, ADC_IT_EOC);

        _value[channel++] = ADC_GetConversionValue(ADCx);
        
        if (channel >= _channels)
        {
            channel = 0;
            adc_xferdone_callback();
        }
    }
    
    /* leave interrupt */
    rt_interrupt_leave();
}
#endif

void adc_xfer_start(void)
{
#ifdef ADC_USE_DMA
    ADC_DMACmd(ADCx, ENABLE);
#endif
    ADC_SoftwareStartConv(ADCx);
}

void adc_xfer_stop(void)
{
#ifdef ADC_USE_DMA
    ADC_DMACmd(ADCx, DISABLE);
#endif
}

void adc_xfer_config(unsigned short *value, unsigned char channels)
{
    ADC_Configuration(channels);
#ifdef ADC_USE_DMA
    DMA_Configuration(value, channels);
#else
    _value = value;
    _channels = channels;
    ADC_ITConfig(ADCx, ADC_IT_EOC, ENABLE);
    ADC_EOCOnEachRegularChannelCmd(ADCx, ENABLE);
#endif
}

int stm32_adc_init(void)
{
    RCC_Configuration();
    GPIO_Configuration();
    NVIC_Configuration();

    return 0;
}
INIT_BOARD_EXPORT(stm32_adc_init);
