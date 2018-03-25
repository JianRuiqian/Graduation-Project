#ifndef __DRV_ADC_H__
#define __DRV_ADC_H__

//#define ADC_USE_DMA

void adc_xfer_start(void);
void adc_xfer_stop(void);
void adc_xfer_config(unsigned short *buf, unsigned char channels);

int stm32_adc_init(void);

#endif  /* __DRV_ADC_H__ */
