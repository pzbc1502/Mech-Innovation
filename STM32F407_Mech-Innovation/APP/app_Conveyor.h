#ifndef __APP_CONVEYOR_H
#define __APP_CONVEYOR_H

#include "main.h"
#include "Conveyor_belt.h"
#include <stdbool.h>

typedef enum {
    MODE_IDLE,
    MODE_AUTO_CLEAN,
    MODE_MAINTENANCE,
    MODE_ERROR
} SystemMode_t;

/* Third-generation automatic process states. */
typedef enum {
    STEP_READY,

    STEP_UPPER_RUN,
    STEP_MIDDLE_RUN,
    STEP_MIDDLE_CUTTING,

    STEP_LOWER_WEIGH,
    STEP_LOWER_PUSH_AND_PACK,
    STEP_PACK_TAIL,
    STEP_LOWER_RETURN,
    STEP_WAIT_NEXT_CYCLE,

    STEP_STOP_UPPER,
    STEP_WAIT_EMPTY_1,
    STEP_STOP_MIDDLE,
    STEP_WAIT_EMPTY_2,
    STEP_STOP_LOWER,

    STEP_DONE
} AutoProcessState_t;

typedef struct {
    SystemMode_t currentMode;
    AutoProcessState_t autoState;

    uint32_t stateTimer;
    uint32_t stateStartTick;
    uint32_t lastTaskTick;
    uint32_t runTime;
    bool isPaused;
    bool stateEntered;

    uint8_t weighSampleCount;
    uint8_t weighValidCount;
    uint16_t weighAttemptCount;
    uint32_t weighLastSampleTick;
    float weighLeftSum;
    float weighRightSum;
    float weighTotalSum;
    float lastWeightLeft;
    float lastWeightRight;
    float lastWeightTotal;

    struct {
        float upperSpeed;
        float middleSpeed;
        float lowerScrewSpeed;
        float packMotorSpeed;
        uint32_t startDelayMs;
        uint32_t stopDelayMs;
        uint32_t runDuration;
        uint32_t weighTime;
        uint32_t cutTime;
        uint32_t washTime;
        uint32_t servoTime;
        uint32_t lowerPushTimeoutMs;
        uint32_t packTailTimeMs;
        uint32_t lowerReturnTimeoutMs;
        uint32_t cycleDelayMs;
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
