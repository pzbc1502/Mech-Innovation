#include "servo.h"
#include "tim.h"

// 静态配置（你可根据舵机手册修改）
static ServoConfig_t s_servo_cfg = {
    .min_pulse_us = 500,
    .max_pulse_us = 2500,
    .center_pulse_us = 1500,
};

/*
    PE9      ------> TIM1_CH1		------> 下层的舵机1
    PE11     ------> TIM1_CH2		------> 下层的舵机2
    PE13     ------> TIM1_CH3		------> 中间层的舵机1
    PE14     ------> TIM1_CH3		------> 中间层的舵机2
*/


/**
 * @brief 初始化舵机PWM（启动TIM1 CH1~CH4）
 * 需确保TIM1已在CubeMX中配置好：PSC=167, ARR=19999, 1tick=1us
 */
void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    __HAL_TIM_MOE_ENABLE(&htim1);

    Servo_SetPulse(SERVO_CH1, s_servo_cfg.center_pulse_us);
    Servo_SetPulse(SERVO_CH2, s_servo_cfg.center_pulse_us);
    Servo_SetPulse(SERVO_CH3, s_servo_cfg.center_pulse_us);
    Servo_SetPulse(SERVO_CH4, s_servo_cfg.center_pulse_us);
}

/**
 * @brief 设置舵机角度（浮点版本，0.0 ~ 270.0）
 */
void Servo_SetAngle(ServoChannel_t ch, float angle)
{
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 270.0f) angle = 270.0f;

    uint16_t pulse_us = s_servo_cfg.min_pulse_us +
        (uint16_t)((angle / 270.0f) * (s_servo_cfg.max_pulse_us - s_servo_cfg.min_pulse_us));

    Servo_SetPulse(ch, pulse_us);
}

/**
 * @brief 设置舵机角度（整数版本，0 ~ 270，无浮点运算）
 */
void Servo_SetAngleInt(ServoChannel_t ch, uint16_t angle)
{
    if (angle > 270) angle = 270;

    // 使用整数运算：pulse = min + (angle * range) / 270
    uint32_t range = s_servo_cfg.max_pulse_us - s_servo_cfg.min_pulse_us;
    uint16_t pulse_us = s_servo_cfg.min_pulse_us + (uint16_t)((angle * range) / 270);

    Servo_SetPulse(ch, pulse_us);
}

/**
 * @brief 直接设置脉宽（单位：微秒）
 */
void Servo_SetPulse(ServoChannel_t ch, uint16_t pulse_us)
{
    if (pulse_us < s_servo_cfg.min_pulse_us) pulse_us = s_servo_cfg.min_pulse_us;
    if (pulse_us > s_servo_cfg.max_pulse_us) pulse_us = s_servo_cfg.max_pulse_us;

    switch (ch) {
        case SERVO_CH1:
            TIM1->CCR1 = pulse_us;
            break;
        case SERVO_CH2:
            TIM1->CCR2 = pulse_us;
            break;
        case SERVO_CH3:
            TIM1->CCR3 = pulse_us;
            break;
        case SERVO_CH4:
            TIM1->CCR4 = pulse_us;
            break;
        default:
            break;
    }
}

