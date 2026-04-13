#include "Servo_Ctrl.h"

/**
 * @brief 舵机系统初始化
 *        初始化底层PWM，并将所有舵机复位到初始状态
 */
void ServoSystem_Init(void)
{
    // 1. 初始化底层定时器
    Servo_Init();
    
    // 2. 复位所有舵机到关闭状态 (0度)
    Servo_Action(SERVO_LOWER_1,  ACTION_CLOSE);
    Servo_Action(SERVO_LOWER_2,  ACTION_CLOSE);
    Servo_Action(SERVO_MIDDLE_1, ACTION_CLOSE);
    Servo_Action(SERVO_MIDDLE_2, ACTION_CLOSE);
}

/**
 * @brief 单个舵机动作控制
 * @param id: 舵机ID (SERVO_LOWER_1 等)
 * @param action: ACTION_OPEN 或 ACTION_CLOSE
 */
void Servo_Action(ServoID_t id, ServoAction_t action)
{
    uint16_t target_angle = 0;
    
    // 1. 根据动作确定目标角度
    // 如果不同舵机的开合角度不一样，可以在这里用 switch(id) 分别处理
    if (action == ACTION_OPEN) {
        target_angle = ANGLE_OPEN; // 比如90度
    } else {
        target_angle = ANGLE_CLOSE; // 比如0度
    }
    
    // 2. 调用底层驱动
    // 因为 ServoID_t 直接对应 ServoChannel_t，可以直接强转
    Servo_SetAngleInt((ServoChannel_t)id, target_angle);
}

/**
 * @brief 指定任意角度 (调试用)
 */
void Servo_Set_Specific_Angle(ServoID_t id, uint16_t angle)
{
    Servo_SetAngleInt((ServoChannel_t)id, angle);
}

// --- 组合动作接口 (方便App层调用) ---

void Lower_Layer_Open(void) {
    Servo_Action(SERVO_LOWER_1, ACTION_OPEN);
    Servo_Action(SERVO_LOWER_2, ACTION_OPEN);
}


void Lower_Layer_Close(void) {
    Servo_Action(SERVO_LOWER_1, ACTION_CLOSE);
    Servo_Action(SERVO_LOWER_2, ACTION_CLOSE);
}


void Middle_Layer_Open(void) {
    Servo_Action(SERVO_MIDDLE_1, ACTION_OPEN);
    Servo_Action(SERVO_MIDDLE_2, ACTION_OPEN);
}


void Middle_Layer_Close(void) {
    Servo_Action(SERVO_MIDDLE_1, ACTION_CLOSE);
    Servo_Action(SERVO_MIDDLE_2, ACTION_CLOSE);
}
