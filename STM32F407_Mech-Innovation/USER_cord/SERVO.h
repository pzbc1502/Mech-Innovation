#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

typedef enum {
    SERVO_CH1 = 0,
    SERVO_CH2
} ServoChannel_t;

typedef struct {
    uint16_t min_pulse_us;
    uint16_t max_pulse_us;
    uint16_t center_pulse_us;
} ServoConfig_t;

void Servo_Init(void);
void Servo_SetAngle(ServoChannel_t ch, float angle);
void Servo_SetAngleInt(ServoChannel_t ch, uint16_t angle);
void Servo_SetPulse(ServoChannel_t ch, uint16_t pulse_us);

#endif
