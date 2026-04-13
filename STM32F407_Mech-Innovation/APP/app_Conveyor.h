#ifndef __APP_CONVEYOR_H
#define __APP_CONVEYOR_H

#include "main.h"
#include "Conveyor_belt.h"

// 系统运行模式
typedef enum {
    MODE_IDLE,          // 空闲/停止模式
    MODE_AUTO_CLEAN,    // 自动清洗流程
    MODE_MAINTENANCE,   // 维护/手动模式
    MODE_ERROR          // 故障模式
} SystemMode_t;

// 自动流程的子状态（优化后的流水线逻辑）
typedef enum {
    // --- 启动序列 ---
    STEP_READY,
	
    // --- 步骤1：上层冲洗 ---
    STEP_UPPER_RUN,     // 上层传送带运行（把东西运到喷头下）
//    STEP_UPPER_WASH,    // 停止并喷水清洗
    
    // --- 步骤2：中层切割 ---
    STEP_MIDDLE_RUN,    // 中层传送带运行（把东西运到刀下）
//    STEP_MIDDLE_CUT,    // 停止并执行切割动作
    
    // --- 步骤3：下层称重 ---
    STEP_LOWER_RUN_1,     // 下层传送带运行（把东西运到秤上）
    STEP_LOWER_WEIGH,   // 停止并称重
    STEP_LOWER_RUN_2,
    STEP_PACKING,      // 打包流程：先7号电机后8号电机
	STEP_LOWER_RUN_SERVO,
	
    // --- 稳定运行 ---
    STEP_RUNNING,       // 正常运行
    
    // --- 停机序列 (逆序) ---
    STEP_STOP_UPPER,    // 1. 先停上层(不再进料)
    STEP_WAIT_EMPTY_1,  //    等待上层货物排空
    STEP_STOP_MIDDLE,   // 2. 再停中层
    STEP_WAIT_EMPTY_2,  //    等待中层货物排空
    STEP_STOP_LOWER,    // 3. 最后停下层
    
    STEP_DONE           // 完成
} AutoProcessState_t;

// 应用层控制句柄
typedef struct {
    SystemMode_t currentMode;
    AutoProcessState_t autoState;
    
    uint32_t stateTimer;       // 状态机计时器
    uint32_t runTime;          // 运行总时长
    bool isPaused;
    
    // 参数配置
    struct {
        float upperSpeed;
        float middleSpeed;
        float lowerSpeed;
        float packConveyorSpeed; // 7号电机速度(小传送带)
        float packWrapSpeed;     // 8号电机速度(缠绕)
        uint32_t startDelayMs; // 启动层级间隔
        uint32_t stopDelayMs;  // 停机排空时间(关键参数)
		
		uint32_t runDuration;  // 传送带运行时间(假设运行这么多时间能到位)
		uint32_t runDuration_Low;
        uint32_t packMotor7TimeMs; // 7号电机运行时间
        uint32_t packMotor8TimeMs; // 8号电机运行时间
        uint32_t weighTime;    // 称重稳定时间
        uint32_t cutTime;      // 切割动作持续时间
        uint32_t washTime;     // 冲洗持续时间
		uint32_t servoTime;     // 冲洗持续时间
		
    } config;
    
} App_WashingCtrl_t;

// --- 公开函数接口 ---
void App_Conwashing_Init(void);
void App_Conwashing_Task(void);

void App_Send_Cmd_StartAuto(void); 
void App_Send_Cmd_Stop(void);      
void App_Send_Cmd_Emergency(void); 
void App_Send_Cmd_Resume(void);    

SystemMode_t App_Get_CurrentMode(void);

#endif


