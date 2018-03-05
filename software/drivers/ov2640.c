#include <rtdevice.h>
#include "board.h"
#include "ov2640.h"
#include "ov2640cfg.h"

static struct rt_i2c_bus_device *i2c_bus = RT_NULL;

rt_inline int _i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                         unsigned char length, unsigned char const data)
{
    rt_size_t ret;
    struct rt_i2c_msg msg[2];

    msg[0].addr  = slave_addr;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = (rt_uint8_t *)&reg_addr;
    msg[1].addr  = slave_addr;
    msg[1].flags = RT_I2C_WR | RT_I2C_NO_START;
    msg[1].len   = length;
    msg[1].buf   = (rt_uint8_t *)&data;

    ret = rt_i2c_transfer(i2c_bus, msg, 2);

    return (ret == 2) ? 0 : -1;
}

rt_inline int _i2c_read(unsigned char slave_addr, unsigned char reg_addr,
                        unsigned char length, unsigned char *data)
{
    rt_size_t ret;
    struct rt_i2c_msg msg[2];

    msg[0].addr  = slave_addr;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = (rt_uint8_t *)&reg_addr;
    msg[1].addr  = slave_addr;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = length;
    msg[1].buf   = (rt_uint8_t *)data;

    ret = rt_i2c_transfer(i2c_bus, msg, 2);

    return (ret == 2) ? 0 : -1;
}

static int ov2640_wreg(unsigned char reg_addr, unsigned char const data)
{
    return _i2c_write(OV2640_ADDR, reg_addr, 1, data);
}

static int ov2640_rreg(unsigned char reg_addr, unsigned char *data)
{
    return _i2c_read(OV2640_ADDR, reg_addr, 1, data);
}

//OV2640�л�ΪJPEGģʽ
void OV2640_JPEG_Mode(void)
{
    u8 i = 0;
    //����:YUV422��ʽ
    for (i = 0; i < (sizeof(OV2640_YUV422) / 2); i++)
    {
        ov2640_wreg(OV2640_YUV422[i][0], OV2640_YUV422[i][1]);
    }

    //����:���JPEG����
    for (i = 0; i < (sizeof(OV2640_JPEG) / 2); i++)
    {
        ov2640_wreg(OV2640_JPEG[i][0], OV2640_JPEG[i][1]);
    }
}

//OV2640�л�ΪRGB565ģʽ
void OV2640_RGB565_Mode(void)
{
    u8 i = 0;
    //����:RGB565���
    for (i = 0; i < (sizeof(OV2640_RGB565) / 2); i++)
    {
        ov2640_wreg(OV2640_RGB565[i][0], OV2640_RGB565[i][1]);
    }
}

//OV2640�Զ��ع�ȼ�����
//level:0~4
void OV2640_Auto_Exposure(uint8_t level)
{
    uint8_t i;
    uint8_t *p = (uint8_t *)OV2640_AUTOEXPOSURE_LEVEL[level];

    for (i = 0; i < 4; i++)
    {
        ov2640_wreg(p[i * 2], p[i * 2 + 1]);
    }
}

//��ƽ������
//0:�Զ�
//1:̫��sunny
//2,����cloudy
//3,�칫��office
//4,����home
void OV2640_Light_Mode(uint8_t mode)
{
    uint8_t regccval = 0x5e;        //Sunny
    uint8_t regcdval = 0x41;
    uint8_t regceval = 0x54;

    switch (mode)
    {
    case 0://auto
        ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
        ov2640_wreg(0xc7, 0x10);//AWB ON
        return;
    case 2://cloudy
        regccval = 0x65;
        regcdval = 0x41;
        regceval = 0x4f;
        break;
    case 3://office
        regccval = 0x52;
        regcdval = 0x41;
        regceval = 0x66;
        break;
    case 4://home
        regccval = 0x42;
        regcdval = 0x3f;
        regceval = 0x71;
        break;
    }

    ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
    ov2640_wreg(0xc7, 0x40);        //AWB OFF
    ov2640_wreg(0xcc, regccval);
    ov2640_wreg(0xcd, regcdval);
    ov2640_wreg(0xce, regceval);
}

//ɫ������
//0:-2
//1:-1
//2,0
//3,+1
//4,+2
void OV2640_Color_Saturation(uint8_t sat)
{
    uint8_t reg7dval = ((sat + 2) << 4) | 0x08;

    ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
    ov2640_wreg(0x7c, 0x00);
    ov2640_wreg(0x7d, 0x02);
    ov2640_wreg(0x7c, 0x03);
    ov2640_wreg(0x7d, reg7dval);
    ov2640_wreg(0x7d, reg7dval);
}

//��������
//0:(0X00)-2
//1:(0X10)-1
//2,(0X20) 0
//3,(0X30)+1
//4,(0X40)+2
void OV2640_Brightness(uint8_t bright)
{
    ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
    ov2640_wreg(0x7c, 0x00);
    ov2640_wreg(0x7d, 0x04);
    ov2640_wreg(0x7c, 0x09);
    ov2640_wreg(0x7d, bright << 4);
    ov2640_wreg(0x7d, 0x00);
}

//�Աȶ�����
//0:-2
//1:-1
//2,0
//3,+1
//4,+2
void OV2640_Contrast(uint8_t contrast)
{
    uint8_t reg7d0val = 0x20; //Ĭ��Ϊ��ͨģʽ
    uint8_t reg7d1val = 0x20;

    switch (contrast)
    {
    case 0://-2
        reg7d0val = 0x18;
        reg7d1val = 0x34;
        break;
    case 1://-1
        reg7d0val = 0x1c;
        reg7d1val = 0x2a;
        break;
    case 3://1
        reg7d0val = 0x24;
        reg7d1val = 0x16;
        break;
    case 4://2
        reg7d0val = 0x28;
        reg7d1val = 0x0c;
        break;
    }

    ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
    ov2640_wreg(0x7c, 0x00);
    ov2640_wreg(0x7d, 0x04);
    ov2640_wreg(0x7c, 0x07);
    ov2640_wreg(0x7d, 0x20);
    ov2640_wreg(0x7d, reg7d0val);
    ov2640_wreg(0x7d, reg7d1val);
    ov2640_wreg(0x7d, 0x06);
}

//��Ч����
//0:��ͨģʽ
//1,��Ƭ
//2,�ڰ�
//3,ƫ��ɫ
//4,ƫ��ɫ
//5,ƫ��ɫ
//6,����
void OV2640_Special_Effects(uint8_t eft)
{
    uint8_t reg7d0val = 0x00; //Ĭ��Ϊ��ͨģʽ
    uint8_t reg7d1val = 0x80;
    uint8_t reg7d2val = 0x80;

    switch (eft)
    {
    case 1://��Ƭ
        reg7d0val = 0x40;
        break;
    case 2://�ڰ�
        reg7d0val = 0x18;
        break;
    case 3://ƫ��ɫ
        reg7d0val = 0x18;
        reg7d1val = 0x40;
        reg7d2val = 0xc0;
        break;
    case 4://ƫ��ɫ
        reg7d0val = 0x18;
        reg7d1val = 0x40;
        reg7d2val = 0x40;
        break;
    case 5://ƫ��ɫ
        reg7d0val = 0x18;
        reg7d1val = 0xa0;
        reg7d2val = 0x40;
        break;
    case 6://����
        reg7d0val = 0x18;
        reg7d1val = 0x40;
        reg7d2val = 0xa6;
        break;
    }

    ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
    ov2640_wreg(0x7c, 0x00);
    ov2640_wreg(0x7d, reg7d0val);
    ov2640_wreg(0x7c, 0x05);
    ov2640_wreg(0x7d, reg7d1val);
    ov2640_wreg(0x7d, reg7d2val);
}

//��������
//sw:0,�رղ���
//   1,��������(ע��OV2640�Ĳ����ǵ�����ͼ�������)
void OV2640_Color_Bar(uint8_t sw)
{
    uint8_t reg;

    ov2640_wreg(OV2640_RA_DLMT, OV2640_SENSOR);
    ov2640_rreg(0x12, &reg);
    reg &= ~(1 << 1);
    if (sw)
        reg |= 1 << 1;
    ov2640_wreg(0x12, reg);
}

//����ͼ���������
//sx,sy,��ʼ��ַ
//width,height:���(��Ӧ:horizontal)�͸߶�(��Ӧ:vertical)
void OV2640_Window_Set(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint16_t ex;
    uint16_t ey;
    uint8_t  temp;

    ex = sx + width  / 2;       //V*2
    ey = sy + height / 2;

    ov2640_wreg(OV2640_RA_DLMT, OV2640_SENSOR);
    ov2640_rreg(0x03, &temp);   //��ȡVref֮ǰ��ֵ
    temp &= 0xf0;
    temp |= ((ey & 0x03) << 2) | (sy & 0x03);
    ov2640_wreg(0x03, temp);    //����Vref��start��end�����2λ
    ov2640_wreg(0x19, sy >> 2); //����Vref��start��8λ
    ov2640_wreg(0x1a, ey >> 2); //����Vref��end�ĸ�8λ

    ov2640_rreg(0x32, &temp);   //��ȡHref֮ǰ��ֵ
    temp &= 0xc0;
    temp |= ((ex & 0x07) << 3) | (sx & 0x07);
    ov2640_wreg(0x32, temp);    //����Href��start��end�����3λ
    ov2640_wreg(0x17, sx >> 3); //����Href��start��8λ
    ov2640_wreg(0x18, ex >> 3); //����Href��end�ĸ�8λ
}

//����ͼ�������С
//OV2640���ͼ��Ĵ�С(�ֱ���),��ȫ�ɸĺ���ȷ��
//width,height:���(��Ӧ:horizontal)�͸߶�(��Ӧ:vertical),width��height������4�ı���
//����ֵ:0,���óɹ�
//    ����,����ʧ��
int OV2640_OutSize_Set(uint16_t width, uint16_t height)
{
    uint16_t outh;
    uint16_t outw;
    uint8_t  temp;

    if (width  % 4)return 1;
    if (height % 4)return 2;
    outw = width  / 4;
    outh = height / 4;
    ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
    ov2640_wreg(0xe0, 0x04);
    ov2640_wreg(0x5a, outw & 0xff);
    ov2640_wreg(0x5b, outh & 0xff);
    temp  = (outw >> 8) & 0x03;
    temp |= (outh >> 6) & 0x04;
    ov2640_wreg(0x5c, temp);
    ov2640_wreg(0xe0, 0X00);

    return 0;
}

//����ͼ�񿪴���С
//��:OV2640_ImageSize_Setȷ������������ֱ��ʴӴ�С.
//�ú������������Χ������п���,����OV2640_OutSize_Set�����
//ע��:�������Ŀ�Ⱥ͸߶�,������ڵ���OV2640_OutSize_Set�����Ŀ�Ⱥ͸߶�
//     OV2640_OutSize_Set���õĿ�Ⱥ͸߶�,���ݱ��������õĿ�Ⱥ͸߶�,��DSP
//     �Զ��������ű���,������ⲿ�豸.
//width,height:���(��Ӧ:horizontal)�͸߶�(��Ӧ:vertical),width��height������4�ı���
//����ֵ:0,���óɹ�
//    ����,����ʧ��
int OV2640_ImageWin_Set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)
{
    uint16_t hsize;
    uint16_t vsize;
    uint8_t  temp;

    if (width %  4)return 1;
    if (height % 4)return 2;
    hsize = width  / 4;
    vsize = height / 4;
    ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
    ov2640_wreg(0xe0, 0x04);
    ov2640_wreg(0x51, hsize & 0xff);   //����H_SIZE�ĵͰ�λ
    ov2640_wreg(0x52, vsize & 0xff);   //����V_SIZE�ĵͰ�λ
    ov2640_wreg(0x53, offx  & 0xff);   //����offx�ĵͰ�λ
    ov2640_wreg(0x54, offy  & 0xff);   //����offy�ĵͰ�λ
    temp  = (vsize >> 1) & 0x80;
    temp |= (offy  >> 4) & 0x70;
    temp |= (hsize >> 5) & 0x08;
    temp |= (offx  >> 8) & 0x07;
    ov2640_wreg(0x55, temp);                //����H_SIZE/V_SIZE/OFFX,OFFY�ĸ�λ
    ov2640_wreg(0x57, (hsize >> 2) & 0x80); //����H_SIZE/V_SIZE/OFFX,OFFY�ĸ�λ
    ov2640_wreg(0xe0, 0x00);

    return 0;
}

//�ú�������ͼ��ߴ��С,Ҳ������ѡ��ʽ������ֱ���
//UXGA:1600*1200,SVGA:800*600,CIF:352*288
//width,height:ͼ���Ⱥ�ͼ��߶�
//����ֵ:0,���óɹ�
//    ����,����ʧ��
int OV2640_ImageSize_Set(uint16_t width, uint16_t height)
{
    uint8_t temp;

    ov2640_wreg(OV2640_RA_DLMT, OV2640_DSP);
    ov2640_wreg(0xe0, 0x04);
    ov2640_wreg(0xc0, (width) >> 3 & 0xff); //����HSIZE��10:3λ
    ov2640_wreg(0xc1, (height) >> 3 & 0xff);//����VSIZE��10:3λ
    temp  = (width & 0x07) << 3;
    temp |= height & 0x07;
    temp |= (width >> 4) & 0x80;
    ov2640_wreg(0x8c, temp);
    ov2640_wreg(0xe0, 0x00);

    return 0;
}

//OV2640��ʼ��
int ov2640_init(void)
{
    uint8_t i;

    rt_uint8_t ov2640id_h;
    rt_uint8_t ov2640id_l;

    i2c_bus = rt_i2c_bus_device_find("i2c1");
    RT_ASSERT(i2c_bus);

    ov2640_wreg(OV2640_RA_DLMT, OV2640_SENSOR);
    ov2640_rreg(OV2640_SENSOR_PIDH, &ov2640id_h);
    ov2640_rreg(OV2640_SENSOR_PIDL, &ov2640id_l);
    if ((ov2640id_h << 8 | ov2640id_l) == OV2640_PID)
        rt_kprintf("OV2640 detection\n");
    else
    {
        rt_kprintf("unknown camrea : [PID]0x%02x%02x\r\n",
                   ov2640id_h, ov2640id_l);
        return -RT_ERROR;
    }

    //��ʼ�� OV2640,����SVGA�ֱ���(800*600)
    for (i = 0; i < sizeof(OV2640_UXGA) / 2; i++)
    {
        ov2640_wreg(OV2640_UXGA[i][0], OV2640_UXGA[i][1]);
    }

    return RT_EOK;
}
INIT_DEVICE_EXPORT(ov2640_init);

#ifdef RT_USING_FINSH
#include <finsh.h>

void cmd_ov2640(int argc, char *argv[])
{
    if (argc == 2)
    {
        if (rt_strcmp(argv[1], "jpeg") == 0)
            OV2640_JPEG_Mode();
        else if (rt_strcmp(argv[1], "rgb565") == 0)
            OV2640_RGB565_Mode();
    }
    else if (argc == 3)
    {
        if (rt_strcmp(argv[1], "exposure") == 0)
            OV2640_Auto_Exposure(atoi(argv[2]));
        else if (rt_strcmp(argv[1], "colors") == 0)
            OV2640_Color_Saturation(atoi(argv[2]));
        else if (rt_strcmp(argv[1], "brightness") == 0)
            OV2640_Brightness(atoi(argv[2]));
        else if (rt_strcmp(argv[1], "contrast") == 0)
            OV2640_Contrast(atoi(argv[2]));
        else if (rt_strcmp(argv[1], "special") == 0)
            OV2640_Special_Effects(atoi(argv[2]));
        else if (rt_strcmp(argv[1], "light") == 0)
            OV2640_Light_Mode(atoi(argv[2]));
    }
    else
    {
        rt_kprintf("Usage: ov2640 jpeg\n");
        rt_kprintf("              rgb565\n");
        rt_kprintf("              exposure [level: 0~4]\n");
        rt_kprintf("              colors [level: 0~4]\n");
        rt_kprintf("              brightness [level: 0~4]\n");
        rt_kprintf("              contrast [level: 0~4]\n");
        rt_kprintf("              special [effects: 0~6]\n");
        rt_kprintf("              light [mode: 0~4]\n");
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_ov2640, ov2640, camera ov2640 config);
#endif
