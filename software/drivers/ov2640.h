#ifndef _OV2640_H_
#define _OV2640_H_

#include <stdint.h>

#define OV2640_PID                  0x2642
#define OV2640_ADDR                 0x30
#define OV2640_RA_DLMT              0xff   //寄存器bank选择地址
#define OV2640_DSP                  0x00   //选择DSP
#define OV2640_SENSOR               0x01   //选择SENSOR

//OV2640的DSP寄存器地址映射表(RADL_MT=0X00)
#define OV2640_DSP_R_BYPASS         0x05
#define OV2640_DSP_Qs               0x44
#define OV2640_DSP_CTRL             0x50
#define OV2640_DSP_HSIZE1           0x51
#define OV2640_DSP_VSIZE1           0x52
#define OV2640_DSP_XOFFL            0x53
#define OV2640_DSP_YOFFL            0x54
#define OV2640_DSP_VHYX             0x55
#define OV2640_DSP_DPRP             0x56
#define OV2640_DSP_TEST             0x57
#define OV2640_DSP_ZMOW             0x5a
#define OV2640_DSP_ZMOH             0x5b
#define OV2640_DSP_ZMHH             0x5c
#define OV2640_DSP_BPADDR           0x7c
#define OV2640_DSP_BPDATA           0x7d
#define OV2640_DSP_CTRL2            0x86
#define OV2640_DSP_CTRL3            0x87
#define OV2640_DSP_SIZEL            0x8C
#define OV2640_DSP_HSIZE2           0xc0
#define OV2640_DSP_VSIZE2           0xc1
#define OV2640_DSP_CTRL0            0xc2
#define OV2640_DSP_CTRL1            0xc3
#define OV2640_DSP_R_DVP_SP         0xd3
#define OV2640_DSP_IMAGE_MODE       0xda
#define OV2640_DSP_RESET            0xe0
#define OV2640_DSP_MS_SP            0xf0
#define OV2640_DSP_SS_ID            0x7f
#define OV2640_DSP_SS_CTRL          0xf8
#define OV2640_DSP_MC_BIST          0xf9
#define OV2640_DSP_MC_AL            0xfa
#define OV2640_DSP_MC_AH            0xfb
#define OV2640_DSP_MC_D             0xfc
#define OV2640_DSP_P_STATUS         0xfe

//OV2640的SENSOR寄存器地址映射表(RADL_MT=0X01)
#define OV2640_SENSOR_GAIN          0x00
#define OV2640_SENSOR_COM1          0x03
#define OV2640_SENSOR_REG04         0x04
#define OV2640_SENSOR_REG08         0x08
#define OV2640_SENSOR_COM2          0x09
#define OV2640_SENSOR_PIDH          0x0a
#define OV2640_SENSOR_PIDL          0x0b
#define OV2640_SENSOR_COM3          0x0c
#define OV2640_SENSOR_COM4          0x0d
#define OV2640_SENSOR_AEC           0x10
#define OV2640_SENSOR_CLKRC         0x11
#define OV2640_SENSOR_COM7          0x12
#define OV2640_SENSOR_COM8          0x13
#define OV2640_SENSOR_COM9          0x14
#define OV2640_SENSOR_COM10         0x15
#define OV2640_SENSOR_HREFST        0x17
#define OV2640_SENSOR_HREFEND       0x18
#define OV2640_SENSOR_VSTART        0x19
#define OV2640_SENSOR_VEND          0x1a
#define OV2640_SENSOR_MIDH          0x1c
#define OV2640_SENSOR_MIDL          0x1d
#define OV2640_SENSOR_AEW           0x24
#define OV2640_SENSOR_AEB           0x25
#define OV2640_SENSOR_W             0x26
#define OV2640_SENSOR_REG2A         0x2a
#define OV2640_SENSOR_FRARL         0x2b
#define OV2640_SENSOR_ADDVSL        0x2d
#define OV2640_SENSOR_ADDVHS        0x2e
#define OV2640_SENSOR_YAVG          0x2f
#define OV2640_SENSOR_REG32         0x32
#define OV2640_SENSOR_ARCOM2        0x34
#define OV2640_SENSOR_REG45         0x45
#define OV2640_SENSOR_FLL           0x46
#define OV2640_SENSOR_FLH           0x47
#define OV2640_SENSOR_COM19         0x48
#define OV2640_SENSOR_ZOOMS         0x49
#define OV2640_SENSOR_COM22         0x4b
#define OV2640_SENSOR_COM25         0x4e
#define OV2640_SENSOR_BD50          0x4f
#define OV2640_SENSOR_BD60          0x50
#define OV2640_SENSOR_REG5D         0x5d
#define OV2640_SENSOR_REG5E         0x5e
#define OV2640_SENSOR_REG5F         0x5f
#define OV2640_SENSOR_REG60         0x60
#define OV2640_SENSOR_HISTO_LOW     0x61
#define OV2640_SENSOR_HISTO_HIGH    0x62

int  ov2640_init(void);                     //OV2640摄像头初始化
void OV2640_JPEG_Mode(void);                //OV2640切换为JPEG模式
void OV2640_RGB565_Mode(void);              //OV2640切换为RGB565模式
void OV2640_Auto_Exposure(uint8_t level);   //OV2640自动曝光等级设置
void OV2640_Light_Mode(uint8_t mode);       //白平衡设置
void OV2640_Color_Saturation(uint8_t sat);  //色度设置
void OV2640_Brightness(uint8_t bright);     //亮度设置
void OV2640_Contrast(uint8_t contrast);     //对比度设置
void OV2640_Special_Effects(uint8_t eft);   //特效设置
void OV2640_Color_Bar(uint8_t sw);          //彩条测试

void OV2640_Window_Set(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);//设置图像输出窗口
int  OV2640_OutSize_Set(uint16_t width, uint16_t height);   //设置图像输出大小
int  OV2640_ImageWin_Set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);//设置图像开窗大小
int  OV2640_ImageSize_Set(uint16_t width, uint16_t height); //设置图像尺寸大小

#endif  /* _OV2640_H_ */
