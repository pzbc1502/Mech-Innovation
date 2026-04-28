#ifndef __CONVEYOR_BELT_H
#define __CONVEYOR_BELT_H

#include "Emm_V5.h"
#include <string.h>
#include <stdbool.h>

// 电机ID定义
#define MOTOR_UPPER_1      1
#define MOTOR_UPPER_2      2
#define MOTOR_MIDDLE_1     3
#define MOTOR_MIDDLE_2     4
#define MOTOR_LOWER_1      5
#define MOTOR_LOWER_2      6
#define MOTOR_PACK_1       7
#define MOTOR_PACK_2       8
#define BROADCAST_ADDR     0x00

/* Motor drivers are controlled through CAN2 via Emm_V5/can_SendCmd. */

// 传送带层定义
typedef enum {
    LAYER_UPPER = 1,
    LAYER_MIDDLE = 2,
    LAYER_LOWER = 3
} LayerType;

// 电机方向定义
#define DIR_CW    0  // 顺时针
#define DIR_CCW   1  // 逆时针
 
// 最大速度定义 (RPM) - 根据实际需求调整
#define MAX_SPEED_UPPER   2000  // 上层传送带最大速度
#define MAX_SPEED_MIDDLE  2000  // 中层传送带最大速度
#define MAX_SPEED_LOWER   2000  // 下层传送带最大速度
#define MAX_SPEED_PACK_1  1000  // 打包小传送带最大速度
#define MAX_SPEED_PACK_2  1000  // 缠绕打包电机最大速度


#define DEFAULT_ACCEL     20   // 默认加速度

// 电机控制结构
typedef struct {
    uint8_t motorID;           // 电机ID
    uint8_t direction;         // 旋转方向 (0-CW, 1-CCW)
    bool isEnabled;            // 使能状态
    uint16_t targetSpeed;      // 目标速度 (RPM)
    uint16_t actualSpeed;      // 实际速度 (RPM，需要从反馈获取)
    uint16_t maxSpeed;         // 最大允许速度 (RPM)
    bool isStalled;            // 堵转状态
} MotorControl;


// 传送带组结构 (每组包含两个电机)
typedef struct {
    LayerType layerID;         // 传送带层标识
    MotorControl motor1;       // 第一个电机
    MotorControl motor2;       // 第二个电机
    float targetSpeedRpm;      // 目标速度(RPM)
} ConveyorBelt;


// 洗菜机系统结构
typedef struct {
    ConveyorBelt upperBelt;    // 上层传送带
    ConveyorBelt middleBelt;   // 中层传送带
    ConveyorBelt lowerBelt;    // 下层传送带
    MotorControl packMotor1;   // 打包小传送带电机(7号)
    MotorControl packMotor2;   // 缠绕打包电机(8号)
    uint8_t defaultAccel;      // 默认加速度 (0-255)
    bool isInitialized;        // 系统是否已初始化
    bool emergencyStop;        // 紧急停止状态
} WashingSystem;



// 全局系统结构体
extern WashingSystem g_washingSystem; 


/* --- 函数接口声明 --- */

// 系统初始化
void WashingSystem_Init(void);

// 核心控制接口
bool StartConveyorBelt(LayerType layer, float speedRpm);
bool StopConveyorBelt(LayerType layer);
bool StartPackConveyor(float speedRpm);
bool StopPackConveyor(void);
bool StartWrapMotor(float speedRpm);
bool StopWrapMotor(void);

// 安全控制
void EmergencyStopAll(void);
bool ResetEmergencyStop(void);

void StopWashingSystem(void);
	

// 辅助接口
ConveyorBelt* GetConveyorBeltByLayer(LayerType layer);

#endif /* __CONVEYOR_BELT_H */

