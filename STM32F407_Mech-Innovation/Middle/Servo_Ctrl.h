#ifndef __SERVO_CTRL_H
#define __SERVO_CTRL_H

#include "servo.h"

typedef enum {
    SERVO_PACK_CUTTER = SERVO_CH1,
    SERVO_LOWER_1 = SERVO_CH1,
    SERVO_LOWER_2 = SERVO_CH2
} ServoID_t;

typedef enum {
    ACTION_CLOSE = 0,
    ACTION_OPEN = 1
} ServoAction_t;

#define ANGLE_CLOSE 0
#define ANGLE_OPEN  90
#define PACK_CUTTER_RETRACT_ANGLE 0
#define PACK_CUTTER_EXTEND_ANGLE  90

void ServoSystem_Init(void);
void Servo_Action(ServoID_t id, ServoAction_t action);
void Servo_Set_Specific_Angle(ServoID_t id, uint16_t angle);
void Pack_Cutter_Retract(void);
void Pack_Cutter_Extend(void);
void Lower_Layer_Open(void);
void Lower_Layer_Close(void);

#endif
