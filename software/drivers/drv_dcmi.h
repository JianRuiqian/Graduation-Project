#ifndef _DRV_DCMI_H_
#define _DRV_DCMI_H_

#ifdef __cplusplus
extern "C" {
#endif

void dcmi_xfer_start(void);
void dcmi_xfer_stop(void);
void dcmi_xfer_resume(void);
void dcmi_xfer_suspend(void);
void dcmi_xfer_config(void *buf, unsigned long len);
unsigned short dcmi_xfer_remaining(void);

int  stm32_dcmi_init(void);

#ifdef __cplusplus
}
#endif

#endif  /* _DRV_DCMI_H_ */
