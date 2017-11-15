#ifndef __DC_MOTOR_H__
#define __DC_MOTOR_H__

#include <rtthread.h>

#define DC_MOTOR_DIR_BRAKE      0
#define DC_MOTOR_DIR_FORWARD    1
#define DC_MOTOR_DIR_REVERSE    2

#define DC_MOTOR_RATIO_MAX      100

struct dc_motor_configure
{
    rt_uint32_t pwm_clock;
};

struct rt_dc_motor_device
{
    struct rt_device            parent;

    const struct dc_motor_ops  *ops;
    struct dc_motor_configure   config;
};
typedef struct rt_dc_motor_device* rt_dc_motor_t;

struct dc_motor_ops
{
    void (*set_direction)(struct rt_dc_motor_device *motor, int direction);
    void (*set_ratio)(struct rt_dc_motor_device *motor, rt_size_t ratio);

    rt_err_t (*configure)(struct rt_dc_motor_device *motor, struct dc_motor_configure *cfg);
};

rt_err_t rt_dc_motor_register(struct rt_dc_motor_device *motor,
                              const char                *name,
                              rt_uint32_t                flag,
                              void                      *data);

void dc_motor_forward(rt_dc_motor_t motor, rt_uint8_t ratio);
void dc_motor_reverse(rt_dc_motor_t motor, rt_uint8_t ratio);
void dc_motor_brake(rt_dc_motor_t motor);

#endif  /* __DC_MOTOR_H__ */
