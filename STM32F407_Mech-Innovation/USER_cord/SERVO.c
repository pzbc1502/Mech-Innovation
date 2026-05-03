#include "servo.h"
#include "tim.h"

static ServoConfig_t s_servo_cfg = {
    .min_pulse_us = 500,
    .max_pulse_us = 2500,
    .center_pulse_us = 1500,
};

void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

    __HAL_TIM_MOE_ENABLE(&htim1);

    Servo_SetPulse(SERVO_CH1, s_servo_cfg.center_pulse_us);
    Servo_SetPulse(SERVO_CH2, s_servo_cfg.center_pulse_us);
}

void Servo_SetAngle(ServoChannel_t ch, float angle)
{
    uint16_t pulse_us;

    if (angle < 0.0f)
    {
        angle = 0.0f;
    }
    if (angle > 270.0f)
    {
        angle = 270.0f;
    }

    pulse_us = s_servo_cfg.min_pulse_us +
        (uint16_t)((angle / 270.0f) * (s_servo_cfg.max_pulse_us - s_servo_cfg.min_pulse_us));

    Servo_SetPulse(ch, pulse_us);
}

void Servo_SetAngleInt(ServoChannel_t ch, uint16_t angle)
{
    uint32_t range;
    uint16_t pulse_us;

    if (angle > 270)
    {
        angle = 270;
    }

    range = s_servo_cfg.max_pulse_us - s_servo_cfg.min_pulse_us;
    pulse_us = s_servo_cfg.min_pulse_us + (uint16_t)((angle * range) / 270);

    Servo_SetPulse(ch, pulse_us);
}

void Servo_SetPulse(ServoChannel_t ch, uint16_t pulse_us)
{
    if (pulse_us < s_servo_cfg.min_pulse_us)
    {
        pulse_us = s_servo_cfg.min_pulse_us;
    }
    if (pulse_us > s_servo_cfg.max_pulse_us)
    {
        pulse_us = s_servo_cfg.max_pulse_us;
    }

    switch (ch)
    {
        case SERVO_CH1:
            TIM1->CCR1 = pulse_us;
            break;
        case SERVO_CH2:
            TIM1->CCR2 = pulse_us;
            break;
        default:
            break;
    }
}
