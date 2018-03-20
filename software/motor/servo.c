#include <rtthread.h>
#include "servo.h"

static rt_err_t rt_servo_init(struct rt_device *dev)
{
    rt_err_t result = RT_EOK;
    struct rt_servo_device *motor;

    RT_ASSERT(dev != RT_NULL);
    motor = (struct rt_servo_device *)dev;

    /* apply configuration */
    if (motor->ops->configure)
        result = motor->ops->configure(motor, &motor->config);

    return result;
}

static rt_err_t rt_servo_open(struct rt_device *dev, rt_uint16_t oflag)
{
    RT_ASSERT(dev != RT_NULL);

    return RT_EOK;
}

static rt_err_t rt_servo_close(struct rt_device *dev)
{
    RT_ASSERT(dev != RT_NULL);

    return RT_EOK;
}

static rt_err_t rt_servo_control(struct rt_device *dev,
#if RTTHREAD_VERSION < 30000
                                 rt_uint8_t        cmd,
#else
                                 int               cmd,
#endif
                                 void             *args)
{
    struct rt_servo_device *motor;

    RT_ASSERT(dev != RT_NULL);
    motor = (struct rt_servo_device *)dev;

    switch (cmd)
    {
    default :
        break;
    }

    return RT_EOK;
}

rt_err_t rt_servo_register(struct rt_servo_device *motor,
                           const char             *name,
                           rt_uint32_t             flag,
                           void                   *data)
{
    struct rt_device *device;
    RT_ASSERT(motor != RT_NULL);

    device = &(motor->parent);

    device->type        = RT_Device_Class_Miscellaneous;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;

    device->init        = rt_servo_init;
    device->open        = rt_servo_open;
    device->close       = rt_servo_close;
    device->read        = RT_NULL;
    device->write       = RT_NULL;
    device->control     = rt_servo_control;
    device->user_data   = data;

    /* register a miscellaneous device */
    return rt_device_register(device, name, flag);
}

rt_err_t servo_move(rt_servo_t motor, rt_int16_t pw_offset)
{
    rt_uint16_t pw;
    
    RT_ASSERT(motor != RT_NULL);
    
    pw = motor->config.pw_mid + pw_offset;
    
    if (pw >= motor->config.pw_min && pw <= motor->config.pw_max)
    {
        motor->ops->set_pulsewidth(motor, pw);
    }
    else
    {
        rt_kprintf("%s: pulse width(%d) out of range. min:%d max:%d\n", 
                   motor->parent.parent.name, pw, 
                   motor->config.pw_min, motor->config.pw_max);
        
        return -RT_ERROR;
    }
    
    return RT_EOK;
}
