#ifndef __SERVO_CTRL_H
#define __SERVO_CTRL_H

#include "servo.h"

// --- 舵机角色定义 (业务名称) ---
typedef enum {
    SERVO_LOWER_1  = SERVO_CH1,  // 下层舵机1
    SERVO_LOWER_2  = SERVO_CH2,  // 下层舵机2
    SERVO_MIDDLE_1 = SERVO_CH3,  // 中层舵机1
    SERVO_MIDDLE_2 = SERVO_CH4   // 中层舵机2
} ServoID_t;

// --- 动作状态定义 ---
typedef enum {
    ACTION_CLOSE = 0, // 关闭/复位状态 (比如0度)
    ACTION_OPEN  = 1  // 打开/工作状态 (比如90度或180度)
} ServoAction_t;

// --- 角度参数配置 (根据机械结构调整) ---
// 假设 0度是关闭，90度是打开，或者自定义
#define ANGLE_CLOSE     0
#define ANGLE_OPEN      90 
// 如果不同舵机的角度不一样，可以在C文件中单独配置

// --- 接口函数 ---
void ServoSystem_Init(void); // 系统初始化
void Servo_Action(ServoID_t id, ServoAction_t action); // 执行动作
void Servo_Set_Specific_Angle(ServoID_t id, uint16_t angle); // 调试用：指定角度

// 高级组合动作 (可选)
void Lower_Layer_Open(void);  // 下层两个舵机同时打开
void Lower_Layer_Close(void); // 下层两个舵机同时关闭
void Middle_Layer_Open(void); // 中层两个舵机同时打开
void Middle_Layer_Close(void); // 中层两个舵机同时关闭

#endif
