#include "Servo_Ctrl.h"

void ServoSystem_Init(void)
{
    Servo_Init();
    Servo_Action(SERVO_LOWER_1, ACTION_CLOSE);
    Servo_Action(SERVO_LOWER_2, ACTION_CLOSE);
}

void Servo_Action(ServoID_t id, ServoAction_t action)
{
    uint16_t target_angle = (action == ACTION_OPEN) ? ANGLE_OPEN : ANGLE_CLOSE;

    Servo_SetAngleInt((ServoChannel_t)id, target_angle);
}

void Servo_Set_Specific_Angle(ServoID_t id, uint16_t angle)
{
    Servo_SetAngleInt((ServoChannel_t)id, angle);
}

void Lower_Layer_Open(void)
{
    Servo_Action(SERVO_LOWER_1, ACTION_OPEN);
    Servo_Action(SERVO_LOWER_2, ACTION_OPEN);
}

void Lower_Layer_Close(void)
{
    Servo_Action(SERVO_LOWER_1, ACTION_CLOSE);
    Servo_Action(SERVO_LOWER_2, ACTION_CLOSE);
}
