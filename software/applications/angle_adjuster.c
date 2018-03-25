#include <rtthread.h>
#include <rtdevice.h>
#include <servo.h>
#include <pid.h>
/* TODO: remove this file */
#include "drv_adc.h"

#define ADC_SAMPLE_TIMES        10
#define ADC_NUM_OF_CHANNELS     4

#define PID_OUTPUT_LIMIT        500
#define PID_POS_KP              0.1f
#define PID_POS_KI              0.0f
#define PID_POS_KD              0.0f

static struct rt_completion adc_done;

void adc_xferdone_callback(void)
{
    rt_completion_done(&adc_done);
}

static rt_err_t angle_adjuster_pid_init(pid_t *pid)
{
    *pid = pid_create();
    if (*pid == RT_NULL)
    {
        rt_kprintf("pid NULL\n");
        return -RT_ENOMEM;
    }
    pid_set_output_limit(*pid, -PID_OUTPUT_LIMIT, PID_OUTPUT_LIMIT);
    pid_config(*pid, PID_POS_KP, PID_POS_KI, PID_POS_KD);

    return RT_EOK;;
}

static void angle_adjuster_thread_entry(void *parameters)
{
    pid_t pid_xaxis;
    pid_t pid_yaxis;
    rt_int16_t pw_xaxis;
    rt_int16_t pw_yaxis;
    rt_device_t servo_xasis;
    rt_device_t servo_yasis;

    /* pid init */
    angle_adjuster_pid_init(&pid_xaxis);
    angle_adjuster_pid_init(&pid_yaxis);
    /* servo init */
    servo_xasis = rt_device_find("servo1");
    servo_yasis = rt_device_find("servo2");
    rt_device_open(servo_xasis, RT_DEVICE_OFLAG_RDWR);
    rt_device_open(servo_yasis, RT_DEVICE_OFLAG_RDWR);
    /* adc completion init */
    rt_completion_init(&adc_done);

    while (1)
    {
        rt_uint8_t times;
        rt_uint8_t channels;
        rt_uint16_t value[ADC_NUM_OF_CHANNELS];
        rt_uint32_t sum[ADC_NUM_OF_CHANNELS] = {0};

        /* collect adc value */
        for (times = 0; times < ADC_SAMPLE_TIMES; times++)
        {
            adc_xfer_config(value, ADC_NUM_OF_CHANNELS);
            adc_xfer_start();
            rt_completion_wait(&adc_done, RT_WAITING_FOREVER);
            adc_xfer_stop();

            for (channels = 0; channels < ADC_NUM_OF_CHANNELS; channels++)
            {
                sum[channels] += value[channels];
            }
        }
        /* mean filter */
        for (channels = 0; channels < ADC_NUM_OF_CHANNELS; channels++)
        {
            value[channels] = sum[channels] / ADC_SAMPLE_TIMES;
        }
        /* pid control */
        pw_xaxis = pid_position_ctrl(pid_xaxis, 0, value[0] - value[1]);
        pw_yaxis = pid_position_ctrl(pid_yaxis, 0, value[2] - value[3]);
        /* servo move */
        servo_move((rt_servo_t)servo_xasis, pw_xaxis);
        servo_move((rt_servo_t)servo_yasis, pw_yaxis);

        rt_thread_delay(rt_tick_from_millisecond(200));
    }
}

int angle_adjuster_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("adjuster", angle_adjuster_thread_entry,
                           RT_NULL, 512, 25, 10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);

    return 0;
}
INIT_APP_EXPORT(angle_adjuster_init);
