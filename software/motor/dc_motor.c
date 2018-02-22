#include <rtthread.h>
#include "dc_motor.h"

static rt_err_t rt_dc_motor_init(struct rt_device *dev)
{
    rt_err_t result = RT_EOK;
    struct rt_dc_motor_device *motor;

    RT_ASSERT(dev != RT_NULL);
    motor = (struct rt_dc_motor_device *)dev;

    /* apply configuration */
    if (motor->ops->configure)
        result = motor->ops->configure(motor, &motor->config);

    return result;
}

static rt_err_t rt_dc_motor_open(struct rt_device *dev, rt_uint16_t oflag)
{
    RT_ASSERT(dev != RT_NULL);

    return RT_EOK;
}

static rt_err_t rt_dc_motor_close(struct rt_device *dev)
{
    RT_ASSERT(dev != RT_NULL);

    return RT_EOK;
}

static rt_err_t rt_dc_motor_control(struct rt_device *dev,
                                    rt_uint8_t        cmd,
                                    void             *args)
{
    struct rt_dc_motor_device *motor;

    RT_ASSERT(dev != RT_NULL);
    motor = (struct rt_dc_motor_device *)dev;

    switch (cmd)
    {
    default :
        break;
    }

    return RT_EOK;
}

rt_err_t rt_dc_motor_register(struct rt_dc_motor_device *motor,
                              const char                *name,
                              rt_uint32_t                flag,
                              void                      *data)
{
    struct rt_device *device;
    RT_ASSERT(motor != RT_NULL);

    device = &(motor->parent);

    device->type        = RT_Device_Class_Miscellaneous;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;

    device->init        = rt_dc_motor_init;
    device->open        = rt_dc_motor_open;
    device->close       = rt_dc_motor_close;
    device->read        = RT_NULL;
    device->write       = RT_NULL;
    device->control     = rt_dc_motor_control;
    device->user_data   = data;

    /* register a miscellaneous device */
    return rt_device_register(device, name, flag);
}

void dc_motor_forward(rt_dc_motor_t motor, rt_uint8_t ratio)
{
    RT_ASSERT(motor != RT_NULL);

    if (ratio <= DC_MOTOR_RATIO_MAX)
    {
        motor->ops->set_direction(motor, DC_MOTOR_DIR_FORWARD);
        motor->ops->set_ratio(motor, ratio);
    }
    else
    {
        rt_kprintf("%s: ratio(%d) larger than DC_MOTOR_RATIO_MAX(%d)\n",
                   motor->parent.parent.name, ratio, DC_MOTOR_RATIO_MAX);
    }
}

void dc_motor_reverse(rt_dc_motor_t motor, rt_uint8_t ratio)
{
    RT_ASSERT(motor != RT_NULL);

    if (ratio <= DC_MOTOR_RATIO_MAX)
    {
        motor->ops->set_direction(motor, DC_MOTOR_DIR_REVERSE);
        motor->ops->set_ratio(motor, ratio);
    }
    else
    {
        rt_kprintf("%s: ratio(%d) larger than DC_MOTOR_RATIO_MAX(%d)\n",
                   motor->parent.parent.name, ratio, DC_MOTOR_RATIO_MAX);
    }
}

void dc_motor_brake(rt_dc_motor_t motor)
{
    motor->ops->set_direction(motor, DC_MOTOR_DIR_BRAKE);
    motor->ops->set_ratio(motor, 0);
}
