#include "Servo_Ctrl.h"

void ServoSystem_Init(void)
{
    Servo_Init();
    Pack_Cutter_Retract();
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

void Pack_Cutter_Retract(void)
{
    Servo_SetAngleInt((ServoChannel_t)SERVO_PACK_CUTTER, PACK_CUTTER_RETRACT_ANGLE);
}

void Pack_Cutter_Extend(void)
{
    Servo_SetAngleInt((ServoChannel_t)SERVO_PACK_CUTTER, PACK_CUTTER_EXTEND_ANGLE);
}

void Lower_Layer_Open(void)
{
    Pack_Cutter_Extend();
}

void Lower_Layer_Close(void)
{
    Pack_Cutter_Retract();
}
