#ifndef __CONVEYOR_BELT_H
#define __CONVEYOR_BELT_H

#include "Emm_V5.h"
#include <string.h>
#include <stdbool.h>

/* 电机通信链路为 CAN2 -> Emm_V5 -> can_SendCmd。
 * 第三代流程只调度 1-7 号电机，8 号电机故意不参与控制。
 */

// 电机ID定义
#define MOTOR_UPPER_1          1
#define MOTOR_UPPER_2          2
#define MOTOR_MIDDLE_1         3
#define MOTOR_MIDDLE_2         4
#define MOTOR_LOWER_SCREW_1    5
#define MOTOR_LOWER_SCREW_2    6
#define MOTOR_PACK             7
#define BROADCAST_ADDR         0x00

// 传送带层定义：第三代只保留上层/中层传送带
typedef enum {
    LAYER_UPPER = 1,
    LAYER_MIDDLE = 2
} LayerType;

// 电机方向定义
#define DIR_CW    0
#define DIR_CCW   1

// 最大速度定义 (RPM)
#define MAX_SPEED_UPPER        2000
#define MAX_SPEED_MIDDLE       2000
#define MAX_SPEED_LOWER_SCREW  1000
#define MAX_SPEED_PACK         1000

#define DEFAULT_ACCEL          20
#define LOWER_SCREW_ACCEL      0
#define LOWER_SCREW_PULSES     240000UL  // 300mm 行程、8mm 导程对应的位置模式目标脉冲

// 电机控制结构
typedef struct {
    uint8_t motorID;
    uint8_t direction;
    bool isEnabled;
    uint16_t targetSpeed;
    uint16_t actualSpeed;
    uint16_t maxSpeed;
    bool isStalled;
} MotorControl;

// 传送带组结构 (每组包含两个电机)
typedef struct {
    LayerType layerID;
    MotorControl motor1;
    MotorControl motor2;
    float targetSpeedRpm;
} ConveyorBelt;

// 第三层丝杆推板结构：5/6号电机位置模式同步，按时间估算到位
typedef struct {
    MotorControl motor1;
    MotorControl motor2;
    uint32_t targetPulses;
    uint16_t targetSpeedRpm;
    bool isMoving;
} LowerScrewPush;

// 洗菜机系统结构
typedef struct {
    ConveyorBelt upperBelt;
    ConveyorBelt middleBelt;
    LowerScrewPush lowerScrew;
    MotorControl packMotor;      // 7号旋转打包电机
    uint8_t defaultAccel;
    bool isInitialized;
    bool emergencyStop;
} WashingSystem;

extern WashingSystem g_washingSystem;

// 系统初始化
void WashingSystem_Init(void);

// 上层/中层传送带控制
bool StartConveyorBelt(LayerType layer, float speedRpm);
bool StopConveyorBelt(LayerType layer);

// 第三层丝杆推板控制
bool StartLowerPush(float speedRpm);

// 推板推动和7号打包电机同一次广播同步启动
bool StartLowerPushAndPack(float screwSpeedRpm, float packSpeedRpm);
bool StartLowerReturn(float speedRpm);
bool StopLowerScrew(void);

// 7号旋转打包电机控制
bool StartPackMotor(float speedRpm);
bool StopPackMotor(void);

// 安全控制
void EmergencyStopAll(void);
bool ResetEmergencyStop(void);
void StopWashingSystem(void);

// 辅助接口
ConveyorBelt* GetConveyorBeltByLayer(LayerType layer);

#endif /* __CONVEYOR_BELT_H */
