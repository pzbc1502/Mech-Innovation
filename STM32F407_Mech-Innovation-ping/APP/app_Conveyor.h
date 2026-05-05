#ifndef __APP_CONVEYOR_H
#define __APP_CONVEYOR_H

#include "main.h"
#include "Conveyor_belt.h"
#include <stdbool.h>

/* 系统运行模式。 */
typedef enum {
    MODE_IDLE,          /* 空闲模式：等待按键或蓝牙命令。 */
    MODE_AUTO_CLEAN,    /* 自动清洗流程运行中。 */
    MODE_MAINTENANCE,   /* 维护模式：预留给单独调试执行机构。 */
    MODE_ERROR          /* 错误/急停模式：需要复位后才能重新启动。 */
} SystemMode_t;

/* 第三代自动流程状态。
 * 第三层不再是普通下层传送带，而是：称重板 + 5/6号丝杆推板 + 7号打包电机。
 */
typedef enum {
    STEP_READY,                 /* 每轮开始前的安全复位状态。 */

    STEP_UPPER_RUN,             /* 上层传送带运行并进行清洗。 */
    STEP_MIDDLE_RUN,            /* 中层传送带运行，等待物料进入切割区域。 */
    STEP_MIDDLE_CUTTING,        /* 中层切割阶段，切刀打开。 */

    STEP_LOWER_WEIGH,           /* 第三层称重，读取左右两路 HX711。 */
    STEP_LOWER_PUSH_AND_PACK,   /* 5/6号丝杆推板与7号打包电机同步启动。 */
    STEP_PACK_TAIL,             /* 推板停止后，PE9舵机剪断包料，7号电机继续运行。 */
    STEP_LOWER_RETURN,          /* 5/6号丝杆推板返回初始位置。 */
    STEP_WAIT_NEXT_CYCLE,       /* 下一轮循环前的短等待。 */

    STEP_STOP_UPPER,            /* 优雅停机：先停止上层进料。 */
    STEP_WAIT_EMPTY_1,          /* 等待已进入中层的物料继续处理。 */
    STEP_STOP_MIDDLE,           /* 优雅停机：停止中层并关闭切刀/下料机构。 */
    STEP_WAIT_EMPTY_2,          /* 等待第三层当前动作进入安全点。 */
    STEP_STOP_LOWER,            /* 优雅停机：停止丝杆推板和打包电机。 */

    STEP_DONE                   /* 流程完成，回到空闲模式。 */
} AutoProcessState_t;

/* 应用层控制句柄。
 * 这里保存状态机、真实计时、称重累加器和自动流程参数。
 */
typedef struct {
    SystemMode_t currentMode;
    AutoProcessState_t autoState;

    uint32_t stateTimer;        /* 当前状态已运行时间，单位 ms。 */
    uint32_t stateStartTick;    /* 当前状态进入时的 HAL tick。 */
    uint32_t lastTaskTick;      /* 上一次调度 tick，用于累计总运行时间。 */
    uint32_t runTime;           /* 自动模式累计运行时间。 */
    bool isPaused;
    bool stateEntered;          /* 进入新状态后的第一个调度周期为 true。 */

    uint8_t weighSampleCount;   /* 已采集到的有效称重样本数。 */
    uint8_t weighValidCount;    /* 参与平均的有效样本数。 */
    uint16_t weighAttemptCount; /* 称重尝试次数，用于避免未接模块时无限等待。 */
    uint32_t weighLastSampleTick;
    float weighLeftSum;
    float weighRightSum;
    float weighTotalSum;
    float lastWeightLeft;
    float lastWeightRight;
    float lastWeightTotal;

    struct {
        float upperSpeed;             /* 上层传送带默认速度，单位 RPM。 */
        float middleSpeed;            /* 中层传送带默认速度，单位 RPM。 */
        float lowerScrewSpeed;        /* 5/6号丝杆推板速度，单位 RPM。 */
        float packMotorSpeed;         /* 7号打包电机速度，单位 RPM。 */
        uint32_t startDelayMs;        /* 中层启动到切割开始的等待时间。 */
        uint32_t stopDelayMs;         /* 优雅停机各层排空等待时间。 */
        uint32_t upperRunDuration;    /* 上层清洗运行时长。 */
        uint32_t middleRunDuration;   /* 中层切割运行时长。 */
        uint32_t weighTime;           /* 称重前的稳定等待时间。 */
        uint32_t cutTime;             /* 切刀动作预留参数。 */
        uint32_t washTime;            /* 清洗动作预留参数。 */
        uint32_t servoTime;           /* 舵机动作预留参数，打包切断由 packTailTimeMs 控制。 */
        uint32_t lowerPushTimeoutMs;  /* 推板推动到位估算时间，当前不解析 CAN 到位反馈。 */
        uint32_t packTailTimeMs;      /* PE9舵机剪断包料并保持7号电机运行的时间。 */
        uint32_t lowerReturnTimeoutMs;/* 推板返回到位估算时间，当前不依赖限位开关。 */
        uint32_t cycleDelayMs;        /* 两轮自动流程之间的等待时间。 */
    } config;
} App_WashingCtrl_t;

void App_Conwashing_Init(void);
void App_Conwashing_Task(void);

void App_Send_Cmd_StartAuto(void);
void App_Send_Cmd_Stop(void);
void App_Send_Cmd_Emergency(void);
void App_Send_Cmd_Resume(void);

SystemMode_t App_Get_CurrentMode(void);

#endif
