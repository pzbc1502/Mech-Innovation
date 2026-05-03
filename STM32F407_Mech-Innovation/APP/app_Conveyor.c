#include "app_Conveyor.h"
#include "debug.h"
#include "bsp_bluetooth.h"
#include "bsp_relay.h"
#include "HX711.h"
#include "Servo_Ctrl.h"
#include "OLED.h"

/* 应用层唯一控制实例：主循环和按键/蓝牙命令都通过它协调状态。 */
static App_WashingCtrl_t appCtrl;

/* 内部工具函数：只在本文件内使用，避免把状态机细节暴露到外部模块。 */
static void App_Enter_AutoState(AutoProcessState_t nextState);
static bool App_Consume_StateEntry(void);
static bool App_StateTimeout(uint32_t durationMs);
static void App_SetActuatorsSafe(void);
static void App_ResetWeighing(void);
static bool App_ProcessWeighing(float *totalWeight, float *leftWeight, float *rightWeight);
static void Run_Auto_Process_FSM(void);
static void Run_Maintenance_Mode(void);

static void App_Enter_AutoState(AutoProcessState_t nextState)
{
    /* 状态计时统一使用 HAL_GetTick() 的真实经过时间，不依赖固定 10ms 累加。 */
    appCtrl.autoState = nextState;
    appCtrl.stateStartTick = HAL_GetTick();
    appCtrl.stateTimer = 0;
    appCtrl.stateEntered = true;
}

static bool App_Consume_StateEntry(void)
{
    /* 每个状态只在首次进入时执行一次启动动作，后续调度只检查超时条件。 */
    if (appCtrl.stateEntered)
    {
        appCtrl.stateEntered = false;
        return true;
    }
    return false;
}

static bool App_StateTimeout(uint32_t durationMs)
{
    return appCtrl.stateTimer >= durationMs;
}

static void App_SetActuatorsSafe(void)
{
    /* 普通停止、急停、故障恢复都会复用这组安全输出状态。 */
    Set_Relay_Switch(RELAY_PUMP, 0);
    Set_Relay_Switch(RELAY_CUTTER, 0);
    Lower_Layer_Close();
}

static void App_ResetWeighing(void)
{
    /* 新一轮称重开始前，清空累加器，避免沿用上一轮残留重量。 */
    appCtrl.weighSampleCount = 0;
    appCtrl.weighValidCount = 0;
    appCtrl.weighAttemptCount = 0;
    appCtrl.weighLastSampleTick = 0;
    appCtrl.weighLeftSum = 0.0f;
    appCtrl.weighRightSum = 0.0f;
    appCtrl.weighTotalSum = 0.0f;
    appCtrl.lastWeightLeft = 0.0f;
    appCtrl.lastWeightRight = 0.0f;
    appCtrl.lastWeightTotal = 0.0f;
}

static bool App_ProcessWeighing(float *totalWeight, float *leftWeight, float *rightWeight)
{
    uint32_t now = HAL_GetTick();
    uint16_t maxAttempts = (uint16_t)(HX711_WEIGHT_SAMPLES * 5U);

    /* 控制采样间隔，避免在一次 10ms 调度内连续读取 HX711。 */
    if ((appCtrl.weighAttemptCount > 0) &&
        ((now - appCtrl.weighLastSampleTick) < HX711_SAMPLE_DELAY_MS))
    {
        return false;
    }

    appCtrl.weighLastSampleTick = now;
    appCtrl.weighAttemptCount++;

    float left = 0.0f;
    float right = 0.0f;
    float total = 0.0f;
    bool leftValid = false;
    bool rightValid = false;

    /* 先检查 HX711 是否就绪，避免未接入的称重模块阻塞 10ms 调度。 */
    if (HX711_IsChannelReady(HX711_CHANNEL_LEFT))
    {
        left = HX711_GetWeightChannel(HX711_CHANNEL_LEFT);
        leftValid = HX711_IsChannelValid(HX711_CHANNEL_LEFT);
    }

    if (HX711_IsChannelReady(HX711_CHANNEL_RIGHT))
    {
        right = HX711_GetWeightChannel(HX711_CHANNEL_RIGHT);
        rightValid = HX711_IsChannelValid(HX711_CHANNEL_RIGHT);
    }

    if (!leftValid)
    {
        left = 0.0f;
    }

    if (!rightValid)
    {
        right = 0.0f;
    }

    total = left + right;

    /* 左右两路谁有效就使用谁；无效或未接入的一侧按 0 处理。 */
    if (leftValid || rightValid)
    {
        appCtrl.weighLeftSum += left;
        appCtrl.weighRightSum += right;
        appCtrl.weighTotalSum += total;
        appCtrl.weighValidCount++;
        appCtrl.weighSampleCount++;
    }

    /* 样本足够或尝试次数耗尽时结束本轮称重，防止传感器缺失时卡死。 */
    if ((appCtrl.weighSampleCount < HX711_WEIGHT_SAMPLES) &&
        (appCtrl.weighAttemptCount < maxAttempts))
    {
        return false;
    }

    if (appCtrl.weighValidCount > 0)
    {
        appCtrl.lastWeightLeft = appCtrl.weighLeftSum / (float)appCtrl.weighValidCount;
        appCtrl.lastWeightRight = appCtrl.weighRightSum / (float)appCtrl.weighValidCount;
        appCtrl.lastWeightTotal = appCtrl.weighTotalSum / (float)appCtrl.weighValidCount;
    }
    else
    {
        appCtrl.lastWeightLeft = 0.0f;
        appCtrl.lastWeightRight = 0.0f;
        appCtrl.lastWeightTotal = 0.0f;
    }

    hx711.Weight_Real = appCtrl.lastWeightTotal;

    if (totalWeight != NULL)
    {
        *totalWeight = appCtrl.lastWeightTotal;
    }
    if (leftWeight != NULL)
    {
        *leftWeight = appCtrl.lastWeightLeft;
    }
    if (rightWeight != NULL)
    {
        *rightWeight = appCtrl.lastWeightRight;
    }

    return true;
}

/**
 * @brief 应用层初始化。
 * @note 初始化底层电机系统、状态机默认参数和蓝牙提示。
 */
void App_Conwashing_Init(void)
{
    WashingSystem_Init();

    memset(&appCtrl, 0, sizeof(App_WashingCtrl_t));
    appCtrl.currentMode = MODE_IDLE;
    App_Enter_AutoState(STEP_READY);
    appCtrl.lastTaskTick = HAL_GetTick();

    appCtrl.config.upperSpeed = 60.0f;
    appCtrl.config.middleSpeed = 60.0f;
    appCtrl.config.lowerScrewSpeed = 30.0f;
    appCtrl.config.packMotorSpeed = 30.0f;

    appCtrl.config.runDuration = 5000;
    appCtrl.config.startDelayMs = 500;
    appCtrl.config.stopDelayMs = 5000;
    appCtrl.config.washTime = 200;
    appCtrl.config.weighTime = 1000;
    appCtrl.config.cutTime = 200;
    appCtrl.config.servoTime = 200;
    /* 当前 v1 不解析 CAN 到位反馈，推板推动/返回按时间估算完成。 */
    /* 30RPM、8mm 导程、300mm 行程约 75s，80s 留少量余量。 */
    appCtrl.config.lowerPushTimeoutMs = 80000;
    appCtrl.config.packTailTimeMs = 3000;
    appCtrl.config.lowerReturnTimeoutMs = 80000;
    appCtrl.config.cycleDelayMs = 600;

#ifdef DEBUG_ENABLE
    printf("[APP] Washing System Initialized\r\n");
#endif
    Bluetooth_SendString("[APP] Washing System Initialized\r\n");
}

/**
 * @brief 应用层主任务，建议每 10ms 调用一次。
 * @note 本函数不主动阻塞等待流程完成，所有阶段由状态机和真实时间推进。
 */
void App_Conwashing_Task(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t delta = 0;

    if (appCtrl.lastTaskTick != 0)
    {
        delta = now - appCtrl.lastTaskTick;
    }
    appCtrl.lastTaskTick = now;
    appCtrl.stateTimer = now - appCtrl.stateStartTick;

    if ((appCtrl.currentMode == MODE_AUTO_CLEAN) && (!appCtrl.isPaused))
    {
        appCtrl.runTime += delta;
    }

    /* 底层急停标志优先级最高，不受当前应用模式影响。 */
    if (g_washingSystem.emergencyStop)
    {
        if (appCtrl.currentMode != MODE_ERROR)
        {
            appCtrl.currentMode = MODE_ERROR;
            App_SetActuatorsSafe();
#ifdef DEBUG_ENABLE
            printf("[APP] EMERGENCY STOP TRIGGERED!\r\n");
#endif
            Bluetooth_SendString("[APP] EMERGENCY STOP TRIGGERED!\r\n");
        }
        return;
    }

    switch (appCtrl.currentMode)
    {
        case MODE_IDLE:
            break;

        case MODE_AUTO_CLEAN:
            Run_Auto_Process_FSM();
            break;

        case MODE_MAINTENANCE:
            Run_Maintenance_Mode();
            break;

        case MODE_ERROR:
            break;

        default:
            break;
    }
}

/**
 * @brief 第三代自动清洗流程状态机。
 * @note 流程为：上层 -> 中层切割 -> 双 HX711 称重 -> 5/6 推板 + 7 打包 -> 推板返回。
 */
static void Run_Auto_Process_FSM(void)
{
    if (appCtrl.isPaused)
    {
        return;
    }

    switch (appCtrl.autoState)
    {
        case STEP_READY:
            /* 每轮自动流程都先把第三层恢复到明确的安全状态。 */
            App_SetActuatorsSafe();
            StopLowerScrew();
            StopPackMotor();
            App_Enter_AutoState(STEP_UPPER_RUN);
#ifdef DEBUG_ENABLE
            printf("[APP] Auto Process Start...\r\n");
#endif
            Bluetooth_SendString("[APP] Auto Process Start...\r\n");
            break;

        case STEP_UPPER_RUN:
            if (App_Consume_StateEntry())
            {
                /* 上层启动：1/2 号电机同步运行，水泵打开。 */
                StartConveyorBelt(LAYER_UPPER, appCtrl.config.upperSpeed);
                Set_Relay_Switch(RELAY_PUMP, 1);
#ifdef DEBUG_ENABLE
                printf("[APP] Upper Process: Belt Running & Pump ON\r\n");
#endif
                Bluetooth_SendString("[APP] Washing Start...\r\n");
            }

            if (App_StateTimeout(appCtrl.config.runDuration))
            {
                StopConveyorBelt(LAYER_UPPER);
                Set_Relay_Switch(RELAY_PUMP, 0);
#ifdef DEBUG_ENABLE
                printf("[APP] Upper Process Done.\r\n");
#endif
                Bluetooth_SendString("[APP] Washing Done.\r\n");
                App_Enter_AutoState(STEP_MIDDLE_RUN);
            }
            break;

        case STEP_MIDDLE_RUN:
            if (App_Consume_StateEntry())
            {
                /* 中层启动：3/4 号电机同步运行，下料舵机打开。 */
                StartConveyorBelt(LAYER_MIDDLE, appCtrl.config.middleSpeed);
                Lower_Layer_Open();
#ifdef DEBUG_ENABLE
                printf("[APP] Middle Process: Belt Running\r\n");
#endif
                Bluetooth_SendString("[APP] Middle Belt Running...\r\n");
            }

            if (App_StateTimeout(appCtrl.config.startDelayMs))
            {
                App_Enter_AutoState(STEP_MIDDLE_CUTTING);
            }
            break;

        case STEP_MIDDLE_CUTTING:
            if (App_Consume_StateEntry())
            {
                /* 中层切割：切刀打开，中层继续运行到设定时长。 */
                Set_Relay_Switch(RELAY_CUTTER, 1);
#ifdef DEBUG_ENABLE
                printf("[APP] Middle Process: Cutter ON\r\n");
#endif
                Bluetooth_SendString("[APP] Cutting Start...\r\n");
            }

            if (App_StateTimeout(appCtrl.config.runDuration))
            {
                StopConveyorBelt(LAYER_MIDDLE);
                Set_Relay_Switch(RELAY_CUTTER, 0);
                Lower_Layer_Close();
#ifdef DEBUG_ENABLE
                printf("[APP] Middle Process Done.\r\n");
#endif
                Bluetooth_SendString("[APP] Cutting Done.\r\n");
                App_Enter_AutoState(STEP_LOWER_WEIGH);
            }
            break;

        case STEP_LOWER_WEIGH:
            if (App_Consume_StateEntry())
            {
                /* 称重板稳定期间，5/6 推板和 7 号打包电机必须保持停止。 */
                StopLowerScrew();
                StopPackMotor();
                App_ResetWeighing();
                Bluetooth_SendString("[APP] Weighing Start...\r\n");
            }

            if (App_StateTimeout(appCtrl.config.weighTime))
            {
                float total = 0.0f;
                float left = 0.0f;
                float right = 0.0f;

                if (App_ProcessWeighing(&total, &left, &right))
                {
#ifdef DEBUG_ENABLE
                    printf("[APP] Weighing Done. L: %.2f g R: %.2f g Total: %.2f g\r\n", left, right, total);
#endif
                    Bluetooth_Printf("[APP] Weighing Done. L: %.2f g R: %.2f g Total: %.2f g\r\n", left, right, total);

                    OLED_ShowString(0, 48, "W:", OLED_8X16);
                    OLED_ShowNum(16, 48, (uint32_t)total, 5, OLED_8X16);
                    OLED_Update();

                    App_Enter_AutoState(STEP_LOWER_PUSH_AND_PACK);
                }
            }
            break;

        case STEP_LOWER_PUSH_AND_PACK:
            if (App_Consume_StateEntry())
            {
                /* 5/6 号位置模式推板和 7 号打包电机通过同一次 CAN 同步广播启动。 */
                StartLowerPushAndPack(appCtrl.config.lowerScrewSpeed, appCtrl.config.packMotorSpeed);
#ifdef DEBUG_ENABLE
                printf("[APP] Lower Push and Pack Start.\r\n");
#endif
                Bluetooth_SendString("[APP] Lower Push and Pack Start.\r\n");
            }

            if (App_StateTimeout(appCtrl.config.lowerPushTimeoutMs))
            {
                StopLowerScrew();
                App_Enter_AutoState(STEP_PACK_TAIL);
            }
            break;

        case STEP_PACK_TAIL:
            if (App_Consume_StateEntry())
            {
                /* 推板到达估算终点后，7 号电机继续短时间旋转完成打包收尾。 */
                Bluetooth_SendString("[APP] Pack Tail Running...\r\n");
            }

            if (App_StateTimeout(appCtrl.config.packTailTimeMs))
            {
                StopPackMotor();
                App_Enter_AutoState(STEP_LOWER_RETURN);
            }
            break;

        case STEP_LOWER_RETURN:
            if (App_Consume_StateEntry())
            {
                /* v1 暂无限位开关和 CAN 到位解析，返回完成按时间估算。 */
                StartLowerReturn(appCtrl.config.lowerScrewSpeed);
#ifdef DEBUG_ENABLE
                printf("[APP] Lower Screw Return Start.\r\n");
#endif
                Bluetooth_SendString("[APP] Lower Screw Return Start.\r\n");
            }

            if (App_StateTimeout(appCtrl.config.lowerReturnTimeoutMs))
            {
                StopLowerScrew();
                App_Enter_AutoState(STEP_WAIT_NEXT_CYCLE);
            }
            break;

        case STEP_WAIT_NEXT_CYCLE:
            if (App_StateTimeout(appCtrl.config.cycleDelayMs))
            {
                App_Enter_AutoState(STEP_UPPER_RUN);
            }
            break;

        case STEP_STOP_UPPER:
            if (App_Consume_StateEntry())
            {
#ifdef DEBUG_ENABLE
                printf("[APP] Stopping Sequence: Stop Upper\r\n");
#endif
                StopConveyorBelt(LAYER_UPPER);
                Set_Relay_Switch(RELAY_PUMP, 0);
                App_Enter_AutoState(STEP_WAIT_EMPTY_1);
            }
            break;

        case STEP_WAIT_EMPTY_1:
            /* 先等待已进入中层的物料继续处理，再停止切割层。 */
            if (App_StateTimeout(appCtrl.config.stopDelayMs))
            {
                App_Enter_AutoState(STEP_STOP_MIDDLE);
            }
            break;

        case STEP_STOP_MIDDLE:
            if (App_Consume_StateEntry())
            {
#ifdef DEBUG_ENABLE
                printf("[APP] Stopping Sequence: Stop Middle\r\n");
#endif
                StopConveyorBelt(LAYER_MIDDLE);
                Set_Relay_Switch(RELAY_CUTTER, 0);
                Lower_Layer_Close();
                App_Enter_AutoState(STEP_WAIT_EMPTY_2);
            }
            break;

        case STEP_WAIT_EMPTY_2:
            /* 最后停止第三层前，给当前动作留出到达安全点的时间。 */
            if (App_StateTimeout(appCtrl.config.stopDelayMs))
            {
                App_Enter_AutoState(STEP_STOP_LOWER);
            }
            break;

        case STEP_STOP_LOWER:
            if (App_Consume_StateEntry())
            {
#ifdef DEBUG_ENABLE
                printf("[APP] Stopping Sequence: Stop Lower.\r\n");
#endif
                StopLowerScrew();
                StopPackMotor();
                App_SetActuatorsSafe();
                App_Enter_AutoState(STEP_DONE);
            }
            break;

        case STEP_DONE:
            App_SetActuatorsSafe();
            StopLowerScrew();
            StopPackMotor();
            appCtrl.currentMode = MODE_IDLE;
            App_Enter_AutoState(STEP_READY);
            break;

        default:
            App_Enter_AutoState(STEP_READY);
            break;
    }
}

/**
 * @brief 维护模式逻辑。
 * @note 当前预留，后续可放入单电机、舵机、继电器等手动测试入口。
 */
static void Run_Maintenance_Mode(void)
{
}

/**
 * @brief 启动自动流程命令。
 */
void App_Send_Cmd_StartAuto(void)
{
    if (appCtrl.currentMode == MODE_IDLE)
    {
        appCtrl.currentMode = MODE_AUTO_CLEAN;
        appCtrl.isPaused = false;
        appCtrl.runTime = 0;
        appCtrl.lastTaskTick = HAL_GetTick();
        App_Enter_AutoState(STEP_READY);
    }
}

/**
 * @brief 停止命令。
 * @note 自动模式下走优雅停机；非自动模式下直接关闭系统并回到空闲。
 */
void App_Send_Cmd_Stop(void)
{
    if (appCtrl.currentMode == MODE_AUTO_CLEAN)
    {
        if (appCtrl.autoState < STEP_STOP_UPPER)
        {
            App_Enter_AutoState(STEP_STOP_UPPER);
        }
    }
    else
    {
        StopWashingSystem();
        App_SetActuatorsSafe();
        appCtrl.currentMode = MODE_IDLE;
        App_Enter_AutoState(STEP_READY);
    }
}

/**
 * @brief 急停命令，立即进入错误模式。
 */
void App_Send_Cmd_Emergency(void)
{
    EmergencyStopAll();
    App_SetActuatorsSafe();
    appCtrl.currentMode = MODE_ERROR;
}

/**
 * @brief 从错误/急停状态恢复到空闲状态。
 */
void App_Send_Cmd_Resume(void)
{
    if (appCtrl.currentMode == MODE_ERROR)
    {
        ResetEmergencyStop();
        App_SetActuatorsSafe();
        appCtrl.currentMode = MODE_IDLE;
        App_Enter_AutoState(STEP_READY);
#ifdef DEBUG_ENABLE
        printf("[APP] Error Resumed.\r\n");
        Bluetooth_SendString("[APP] Error Resumed.\r\n");
#endif
    }
}

SystemMode_t App_Get_CurrentMode(void)
{
    return appCtrl.currentMode;
}
