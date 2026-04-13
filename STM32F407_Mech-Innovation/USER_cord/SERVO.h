#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

// 舵机通道定义
typedef enum {
    SERVO_CH1 = 0,
    SERVO_CH2,
    SERVO_CH3,
	SERVO_CH4
} ServoChannel_t;

// 舵机参数结构体（可选扩展）
typedef struct {
    uint16_t min_pulse_us;  // 最小脉宽，如 500
    uint16_t max_pulse_us;  // 最大脉宽，如 2500
    uint16_t center_pulse_us; // 中间值，如 1500
} ServoConfig_t;

// 函数声明
void Servo_Init(void);
void Servo_SetAngle(ServoChannel_t ch, float angle);
void Servo_SetAngleInt(ServoChannel_t ch, uint16_t angle); // 无浮点版本
void Servo_SetPulse(ServoChannel_t ch, uint16_t pulse_us);

#endif
