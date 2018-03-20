#include <rtthread.h>
#include <servo.h>
#include "board.h"

/* pulse interval in us */
#define MG995_PWM_CYCLE         20000
/* pulse width in us (500-1500-2500) */
#define MG995_PW_MID            (uint32_t)(0.075f * MG995_PWM_CYCLE)
#define MG995_PW_MIN            (uint32_t)(0.025f * MG995_PWM_CYCLE)
#define MG995_PW_MAX            (uint32_t)(0.125f * MG995_PWM_CYCLE)
/* pulse frequence in Hz */
#define MG995_PWM_CLOCK         (1000000 / MG995_PWM_CYCLE)
/* timer frequence in Hz */
#define MG995_TIM_CLOCK         1000000

#define MG995_TIM               TIM5
#define MG995_RCC_APBPeriph_TIM RCC_APB1Periph_TIM5
#define MG995_GPIO_AF_TIM       GPIO_AF_TIM5

#define MG995_CH1_GPIO          GPIOA
#define MG995_CH1_GPIO_PIN      GPIO_Pin_0
#define MG995_CH1_PIN_SOURCE    GPIO_PinSource0
#define MG995_CH1_RCC           RCC_AHB1Periph_GPIOA

#define MG995_CH2_GPIO          GPIOA
#define MG995_CH2_GPIO_PIN      GPIO_Pin_1
#define MG995_CH2_PIN_SOURCE    GPIO_PinSource1
#define MG995_CH2_RCC           RCC_AHB1Periph_GPIOA

#define MG995_CH3_GPIO          GPIOA
#define MG995_CH3_GPIO_PIN      GPIO_Pin_2
#define MG995_CH3_PIN_SOURCE    GPIO_PinSource2
#define MG995_CH3_RCC           RCC_AHB1Periph_GPIOA

struct pwm_channel
{
    TIM_TypeDef *timer;
    void (*channel_init)(TIM_TypeDef *, TIM_OCInitTypeDef *);
    void (*channel_preload_cfg)(TIM_TypeDef *, uint16_t);
    void (*channel_set_compare)(TIM_TypeDef *, uint32_t);
    uint16_t period;
};

#ifdef SERVO_USING_MOTOR1
static struct pwm_channel channel1 =
{
    MG995_TIM,
    TIM_OC1Init,
    TIM_OC1PreloadConfig,
    TIM_SetCompare1,
    MG995_TIM_CLOCK / MG995_PWM_CLOCK,
};

static struct rt_servo_device servo1;

#endif  /* SERVO_USING_MOTOR1 */

#ifdef SERVO_USING_MOTOR2
static struct pwm_channel channel2 =
{
    MG995_TIM,
    TIM_OC2Init,
    TIM_OC2PreloadConfig,
    TIM_SetCompare2,
    MG995_TIM_CLOCK / MG995_PWM_CLOCK,
};

static struct rt_servo_device servo2;

#endif  /* SERVO_USING_MOTOR2 */

#ifdef SERVO_USING_MOTOR3
static struct pwm_channel channel3 =
{
    MG995_TIM,
    TIM_OC3Init,
    TIM_OC3PreloadConfig,
    TIM_SetCompare3,
    MG995_TIM_CLOCK / MG995_PWM_CLOCK,
};

static struct rt_servo_device servo3;

#endif  /* SERVO_USING_MOTOR3 */

static void _set_pulsewidth(struct rt_servo_device *motor, rt_uint16_t pw)
{
    struct pwm_channel *channel;

    RT_ASSERT(motor != RT_NULL);

    channel = (struct pwm_channel *)motor->parent.user_data;
    channel->channel_set_compare(channel->timer, channel->period * pw / MG995_PWM_CYCLE);
}

static rt_err_t _configure(struct rt_servo_device *motor, struct servo_configure *cfg)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;
    struct pwm_channel *channel;
    uint32_t Prescaler;

    RT_ASSERT(motor != RT_NULL);

    channel = (struct pwm_channel *)motor->parent.user_data;
    Prescaler = (SystemCoreClock / 2) / MG995_TIM_CLOCK;

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = channel->period - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = Prescaler - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(channel->timer, &TIM_TimeBaseStructure);

    /* PWM1 Mode configuration */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = 0;

    channel->channel_init(channel->timer, &TIM_OCInitStructure);
    channel->channel_preload_cfg(channel->timer, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(channel->timer, ENABLE);

    /* TIMx enable counter */
    TIM_Cmd(channel->timer, ENABLE);

    return RT_EOK;
}

static struct servo_ops mg995_ops =
{
    _set_pulsewidth,
    _configure,
};

static void RCC_Configuration(void)
{
#ifdef SERVO_USING_MOTOR1
    RCC_AHB1PeriphClockCmd(MG995_CH1_RCC, ENABLE);
#endif  /* SERVO_USING_MOTOR1 */

#ifdef SERVO_USING_MOTOR2
    RCC_AHB1PeriphClockCmd(MG995_CH2_RCC, ENABLE);
#endif  /* SERVO_USING_MOTOR2 */

#ifdef SERVO_USING_MOTOR3
    RCC_AHB1PeriphClockCmd(MG995_CH3_RCC, ENABLE);
#endif  /* SERVO_USING_MOTOR3 */

    RCC_APB1PeriphClockCmd(MG995_RCC_APBPeriph_TIM, ENABLE);
}

static void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

#ifdef SERVO_USING_MOTOR1
    GPIO_InitStructure.GPIO_Pin = MG995_CH1_GPIO_PIN;
    GPIO_Init(MG995_CH1_GPIO, &GPIO_InitStructure);
    /* Connect alternate function */
    GPIO_PinAFConfig(MG995_CH1_GPIO, MG995_CH1_PIN_SOURCE, MG995_GPIO_AF_TIM);
#endif  /* SERVO_USING_MOTOR1 */

#ifdef SERVO_USING_MOTOR2
    GPIO_InitStructure.GPIO_Pin = MG995_CH2_GPIO_PIN;
    GPIO_Init(MG995_CH2_GPIO, &GPIO_InitStructure);
    /* Connect alternate function */
    GPIO_PinAFConfig(MG995_CH2_GPIO, MG995_CH2_PIN_SOURCE, MG995_GPIO_AF_TIM);
#endif  /* SERVO_USING_MOTOR2 */

#ifdef SERVO_USING_MOTOR3
    GPIO_InitStructure.GPIO_Pin = MG995_CH3_GPIO_PIN;
    GPIO_Init(MG995_CH3_GPIO, &GPIO_InitStructure);
    /* Connect alternate function */
    GPIO_PinAFConfig(MG995_CH3_GPIO, MG995_CH3_PIN_SOURCE, MG995_GPIO_AF_TIM);
#endif  /* SERVO_USING_MOTOR3 */
}

int rt_hw_mg995_init(void)
{
    struct pwm_channel *channel;

    RCC_Configuration();
    GPIO_Configuration();

#ifdef SERVO_USING_MOTOR1
    channel = &channel1;

    servo1.config.pw_mid = MG995_PW_MID;
    servo1.config.pw_min = MG995_PW_MIN;
    servo1.config.pw_max = MG995_PW_MAX;
    servo1.ops = &mg995_ops;

    /* register servo1 device */
    rt_servo_register(&servo1, "servo1", RT_DEVICE_FLAG_RDWR, channel);
#endif  /* SERVO_USING_MOTOR1 */

#ifdef SERVO_USING_MOTOR2
    channel = &channel2;

    servo2.config.pw_mid = MG995_PW_MID;
    servo2.config.pw_min = MG995_PW_MIN;
    servo2.config.pw_max = MG995_PW_MAX;
    servo2.ops = &mg995_ops;

    /* register servo2 device */
    rt_servo_register(&servo2, "servo2", RT_DEVICE_FLAG_RDWR, channel);
#endif  /* SERVO_USING_MOTOR2 */

#ifdef SERVO_USING_MOTOR3
    channel = &channel3;

    servo3.config.pw_mid = MG995_PW_MID;
    servo3.config.pw_min = MG995_PW_MIN;
    servo3.config.pw_max = MG995_PW_MAX;
    servo3.ops = &mg995_ops;

    /* register servo3 device */
    rt_servo_register(&servo3, "servo3", RT_DEVICE_FLAG_RDWR, channel);
#endif  /* SERVO_USING_MOTOR3 */

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_mg995_init);

#ifdef RT_USING_FINSH
#include <finsh.h>

void cmd_servo(int argc, char *argv[])
{
    if (argc >= 3)
    {
        int i;

        for (i = 1; i < argc; i++)
        {
            switch (argv[i++][1])
            {
#ifdef SERVO_USING_MOTOR1
            case '1':
                rt_device_open((rt_device_t)&servo1, RT_DEVICE_OFLAG_RDWR);
                servo_move(&servo1, atoi(argv[i]));
                rt_device_close((rt_device_t)&servo1);
                break;
#endif  /* SERVO_USING_MOTOR1 */
#ifdef SERVO_USING_MOTOR2
            case '2':
                rt_device_open((rt_device_t)&servo2, RT_DEVICE_OFLAG_RDWR);
                servo_move(&servo2, atoi(argv[i]));
                rt_device_close((rt_device_t)&servo2);
                break;
#endif  /* SERVO_USING_MOTOR2 */
#ifdef SERVO_USING_MOTOR3
            case '3':
                rt_device_open((rt_device_t)&servo3, RT_DEVICE_OFLAG_RDWR);
                servo_move(&servo3, atoi(argv[i]));
                rt_device_close((rt_device_t)&servo3);
                break;
#endif  /* SERVO_USING_MOTOR3 */
            default:
                break;
            }
        }
    }
    else
        rt_kprintf("Usage: servo [-1..3 pw]\n");
}
MSH_CMD_EXPORT_ALIAS(cmd_servo, servo, notining);
#endif
