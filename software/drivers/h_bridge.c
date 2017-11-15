#include <dc_motor.h>
#include "board.h"

/* H bridge channel1 */
#define H_BRIDGE_CH1_IN1_GPIO       GPIOB
#define H_BRIDGE_CH1_IN1_GPIO_PIN   GPIO_Pin_12
#define H_BRIDGE_CH1_IN1_RCC        RCC_AHB1Periph_GPIOB

#define H_BRIDGE_CH1_IN2_GPIO       GPIOB
#define H_BRIDGE_CH1_IN2_GPIO_PIN   GPIO_Pin_13
#define H_BRIDGE_CH1_IN2_RCC        RCC_AHB1Periph_GPIOB

#define H_BRIDGE_CH1_PWM_GPIO       GPIOD
#define H_BRIDGE_CH1_PWM_GPIO_PIN   GPIO_Pin_12
#define H_BRIDGE_CH1_PWM_PIN_SOURCE GPIO_PinSource12
#define H_BRIDGE_CH1_PWM_RCC        RCC_AHB1Periph_GPIOD
#define H_BRIDGE_CH1_PWM_TIM        TIM4
#define CH1_PWM_RCC_APBPeriph_TIM   RCC_APB1Periph_TIM4
#define CH1_PWM_GPIO_AF_TIM         GPIO_AF_TIM4
#define CH1_PWM_CLOCK               21000

/* H bridge channel2 */
#define H_BRIDGE_CH2_IN1_GPIO       GPIOB
#define H_BRIDGE_CH2_IN1_GPIO_PIN   GPIO_Pin_14
#define H_BRIDGE_CH2_IN1_RCC        RCC_AHB1Periph_GPIOB

#define H_BRIDGE_CH2_IN2_GPIO       GPIOB
#define H_BRIDGE_CH2_IN2_GPIO_PIN   GPIO_Pin_15
#define H_BRIDGE_CH2_IN2_RCC        RCC_AHB1Periph_GPIOB

#define H_BRIDGE_CH2_PWM_GPIO       GPIOD
#define H_BRIDGE_CH2_PWM_GPIO_PIN   GPIO_Pin_13
#define H_BRIDGE_CH2_PWM_PIN_SOURCE GPIO_PinSource13
#define H_BRIDGE_CH2_PWM_RCC        RCC_AHB1Periph_GPIOD
#define H_BRIDGE_CH2_PWM_TIM        TIM4
#define CH2_PWM_RCC_APBPeriph_TIM   RCC_APB1Periph_TIM4
#define CH2_PWM_GPIO_AF_TIM         GPIO_AF_TIM4
#define CH2_PWM_CLOCK               21000

struct pwm_channel
{    
    GPIO_TypeDef* gpio_in1;
    uint16_t pin_in1;
    
    GPIO_TypeDef* gpio_in2;
    uint16_t pin_in2;

    TIM_TypeDef *timer;
    void (*channel_init)(TIM_TypeDef *, TIM_OCInitTypeDef *);
    void (*channel_preload_cfg)(TIM_TypeDef *, uint16_t);
    void (*channel_set_compare)(TIM_TypeDef *, uint32_t);
    uint16_t period;
    uint16_t prescaler;
};

static void _set_direction(struct rt_dc_motor_device *motor, int direction)
{
    struct pwm_channel *channel;
    
    RT_ASSERT(motor != RT_NULL);
    
    channel = (struct pwm_channel *)motor->parent.user_data;
    
    if (direction == DC_MOTOR_DIR_FORWARD)
    {
        GPIO_WriteBit(channel->gpio_in1, channel->pin_in1, Bit_SET);
        GPIO_WriteBit(channel->gpio_in2, channel->pin_in2, Bit_RESET);
    }
    else if (direction == DC_MOTOR_DIR_REVERSE)
    {
        GPIO_WriteBit(channel->gpio_in1, channel->pin_in1, Bit_RESET);
        GPIO_WriteBit(channel->gpio_in2, channel->pin_in2, Bit_SET);
    }
    else if (direction == DC_MOTOR_DIR_BRAKE)
    {
        GPIO_WriteBit(channel->gpio_in1, channel->pin_in1, Bit_RESET);
        GPIO_WriteBit(channel->gpio_in2, channel->pin_in2, Bit_RESET);
    }
}

static void _set_ratio(struct rt_dc_motor_device *motor, rt_size_t ratio)
{
    struct pwm_channel *channel;
    
    RT_ASSERT(motor != RT_NULL);
    
    channel = (struct pwm_channel *)motor->parent.user_data;
    ratio *= 0.01f * channel->period;
    channel->channel_set_compare(channel->timer, ratio);
}

static rt_err_t _configure(struct rt_dc_motor_device *motor, struct dc_motor_configure *cfg)
{
    struct pwm_channel *channel;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;
    
    RT_ASSERT(motor != RT_NULL);

    channel = (struct pwm_channel *)motor->parent.user_data;
    channel->prescaler = ((SystemCoreClock/2) / (cfg->pwm_clock*channel->period));
    
    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = channel->period - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = channel->prescaler - 1;
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

struct dc_motor_ops h_bridge_ops = 
{
    _set_direction,
    _set_ratio,
    _configure,
};

#ifdef DC_MOTOR_USING_MOTOR1
static struct pwm_channel channel1 = 
{
    H_BRIDGE_CH1_IN1_GPIO,
    H_BRIDGE_CH1_IN1_GPIO_PIN,
    H_BRIDGE_CH1_IN2_GPIO,
    H_BRIDGE_CH1_IN2_GPIO_PIN,
    H_BRIDGE_CH1_PWM_TIM,
    TIM_OC1Init,
    TIM_OC1PreloadConfig,
    TIM_SetCompare1,
    100,
};

static struct rt_dc_motor_device dc_motor1;

#endif  /* DC_MOTOR_USING_MOTOR1 */

#ifdef DC_MOTOR_USING_MOTOR2
static struct pwm_channel channel2 = 
{
    H_BRIDGE_CH2_IN1_GPIO,
    H_BRIDGE_CH2_IN1_GPIO_PIN,
    H_BRIDGE_CH2_IN2_GPIO,
    H_BRIDGE_CH2_IN2_GPIO_PIN,
    H_BRIDGE_CH2_PWM_TIM,
    TIM_OC2Init,
    TIM_OC2PreloadConfig,
    TIM_SetCompare2,
    100,
};

static struct rt_dc_motor_device dc_motor2;

#endif  /* DC_MOTOR_USING_MOTOR2 */

static void RCC_Configuration(void)
{
#ifdef DC_MOTOR_USING_MOTOR1
    /* Enable H bridge channel1 inx gpio clocks */
    RCC_AHB1PeriphClockCmd(H_BRIDGE_CH1_IN1_RCC | H_BRIDGE_CH1_IN2_RCC, ENABLE);
    /* Enable H bridge channel1 pwm gpio clock */
    RCC_AHB1PeriphClockCmd(H_BRIDGE_CH1_PWM_RCC, ENABLE);
    /* Enable H bridge channel1 timer clock */
    RCC_APB1PeriphClockCmd(CH1_PWM_RCC_APBPeriph_TIM, ENABLE);
#endif  /* DC_MOTOR_USING_MOTOR1 */

#ifdef DC_MOTOR_USING_MOTOR2
    /* Enable H bridge channel2 inx gpio clocks */
    RCC_AHB1PeriphClockCmd(H_BRIDGE_CH2_IN1_RCC | H_BRIDGE_CH2_IN2_RCC, ENABLE);
    /* Enable H bridge channel2 pwm gpio clock */
    RCC_AHB1PeriphClockCmd(H_BRIDGE_CH2_PWM_RCC, ENABLE);
    /* Enable H bridge channel2 timer clock */
    RCC_APB1PeriphClockCmd(CH2_PWM_RCC_APBPeriph_TIM, ENABLE);
#endif  /* DC_MOTOR_USING_MOTOR2 */
}

static void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

#ifdef DC_MOTOR_USING_MOTOR1
    /* Configure H bridge channel1 inx pins */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin = H_BRIDGE_CH1_IN1_GPIO_PIN;
    GPIO_Init(H_BRIDGE_CH1_IN1_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = H_BRIDGE_CH1_IN2_GPIO_PIN;
    GPIO_Init(H_BRIDGE_CH1_IN2_GPIO, &GPIO_InitStructure);
    
    /* Configure H bridge channel1 pwm pin */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = H_BRIDGE_CH1_PWM_GPIO_PIN;
    GPIO_Init(H_BRIDGE_CH1_PWM_GPIO, &GPIO_InitStructure);

    /* Connect alternate function */
    GPIO_PinAFConfig(H_BRIDGE_CH1_PWM_GPIO, H_BRIDGE_CH1_PWM_PIN_SOURCE, CH1_PWM_GPIO_AF_TIM);
#endif /* DC_MOTOR_USING_MOTOR1 */
    
#ifdef DC_MOTOR_USING_MOTOR2
    /* Configure H bridge channel2 inx pins */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin = H_BRIDGE_CH2_IN1_GPIO_PIN;
    GPIO_Init(H_BRIDGE_CH2_IN1_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = H_BRIDGE_CH2_IN2_GPIO_PIN;
    GPIO_Init(H_BRIDGE_CH2_IN2_GPIO, &GPIO_InitStructure);
    
    /* Configure H bridge channel2 pwm pin */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = H_BRIDGE_CH2_PWM_GPIO_PIN;
    GPIO_Init(H_BRIDGE_CH2_PWM_GPIO, &GPIO_InitStructure);

    /* Connect alternate function */
    GPIO_PinAFConfig(H_BRIDGE_CH2_PWM_GPIO, H_BRIDGE_CH2_PWM_PIN_SOURCE, CH2_PWM_GPIO_AF_TIM);
#endif /* DC_MOTOR_USING_MOTOR2 */
}

int rt_hw_h_bridge_init(void)
{
    struct pwm_channel *channel;
    struct dc_motor_configure config;
    
    RCC_Configuration();
    GPIO_Configuration();
    
#ifdef DC_MOTOR_USING_MOTOR1
    channel = &channel1;
    
    config.pwm_clock = CH1_PWM_CLOCK;
    dc_motor1.config = config;
    dc_motor1.ops = &h_bridge_ops;
    
    /* register dc motor1 device */
    rt_dc_motor_register(&dc_motor1,
                          "motor1",
                          RT_DEVICE_FLAG_RDWR,
                          channel);
#endif  /* DC_MOTOR_USING_MOTOR1 */

#ifdef DC_MOTOR_USING_MOTOR2
    channel = &channel2;
    
    config.pwm_clock = CH2_PWM_CLOCK;
    dc_motor2.config = config;
    dc_motor2.ops = &h_bridge_ops;
    
    /* register dc motor2 device */
    rt_dc_motor_register(&dc_motor2,
                          "motor2",
                          RT_DEVICE_FLAG_RDWR,
                          channel);
#endif  /* DC_MOTOR_USING_MOTOR2 */
    return 0;
}
INIT_BOARD_EXPORT(rt_hw_h_bridge_init);
