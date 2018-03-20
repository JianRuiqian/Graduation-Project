#ifndef __SERVO_H__
#define __SERVO_H__

#include <rtthread.h>

struct servo_configure
{
    rt_uint16_t pw_mid;
    rt_uint16_t pw_min;
    rt_uint16_t pw_max;
};

struct rt_servo_device
{
    struct rt_device        parent;

    const struct servo_ops  *ops;
    struct servo_configure  config;
};
typedef struct rt_servo_device *rt_servo_t;

struct servo_ops
{
    void (*set_pulsewidth)(struct rt_servo_device *motor, rt_uint16_t pw);

    rt_err_t (*configure)(struct rt_servo_device *motor, struct servo_configure *cfg);
};

rt_err_t rt_servo_register(struct rt_servo_device *motor,
                           const char             *name,
                           rt_uint32_t             flag,
                           void                   *data);

rt_err_t servo_move(rt_servo_t motor, rt_int16_t pw_offset);

#endif  /* __SERVO_H__ */
