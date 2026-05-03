#include "Conveyor_belt.h"

// 全局系统结构体
WashingSystem g_washingSystem;

static void ConfigureMotor(MotorControl *motor, uint8_t id, uint16_t maxSpeed, uint8_t direction)
{
    motor->motorID = id;
    motor->maxSpeed = maxSpeed;
    motor->direction = direction;
    motor->targetSpeed = 0;
    motor->actualSpeed = 0;
    motor->isEnabled = false;
    motor->isStalled = false;
}

static uint16_t ClampSpeed(float speedRpm, uint16_t maxSpeed)
{
    if (speedRpm < 0.0f)
    {
        speedRpm = 0.0f;
    }

    if (speedRpm > (float)maxSpeed)
    {
        speedRpm = (float)maxSpeed;
    }

    return (uint16_t)speedRpm;
}

static bool StartLowerScrewMove(uint8_t direction, float speedRpm)
{
    if (g_washingSystem.emergencyStop)
    {
        return false;
    }

    uint16_t rpm = ClampSpeed(speedRpm, MAX_SPEED_LOWER_SCREW);

    g_washingSystem.lowerScrew.motor1.direction = direction;
    g_washingSystem.lowerScrew.motor2.direction = direction;
    g_washingSystem.lowerScrew.motor1.targetSpeed = rpm;
    g_washingSystem.lowerScrew.motor2.targetSpeed = rpm;
    g_washingSystem.lowerScrew.motor1.isEnabled = true;
    g_washingSystem.lowerScrew.motor2.isEnabled = true;
    g_washingSystem.lowerScrew.targetSpeedRpm = rpm;
    g_washingSystem.lowerScrew.targetPulses = LOWER_SCREW_PULSES;
    g_washingSystem.lowerScrew.isMoving = true;

    /* 5/6 号命令使用 snF=true 先缓存，最后用一次广播同时启动。 */
    Emm_V5_En_Control(g_washingSystem.lowerScrew.motor1.motorID, true, true);
    HAL_Delay(50);
    Emm_V5_En_Control(g_washingSystem.lowerScrew.motor2.motorID, true, true);
    HAL_Delay(50);

    Emm_V5_Pos_Control(g_washingSystem.lowerScrew.motor1.motorID,
                       direction,
                       rpm,
                       LOWER_SCREW_ACCEL,
                       LOWER_SCREW_PULSES,
                       false,
                       true);
    HAL_Delay(50);
    Emm_V5_Pos_Control(g_washingSystem.lowerScrew.motor2.motorID,
                       direction,
                       rpm,
                       LOWER_SCREW_ACCEL,
                       LOWER_SCREW_PULSES,
                       false,
                       true);
    HAL_Delay(50);
    Emm_V5_Synchronous_motion(BROADCAST_ADDR);

    return true;
}

static bool StartLowerPushAndPackMove(float screwSpeedRpm, float packSpeedRpm)
{
    if (g_washingSystem.emergencyStop)
    {
        return false;
    }

    uint16_t screwRpm = ClampSpeed(screwSpeedRpm, MAX_SPEED_LOWER_SCREW);
    uint16_t packRpm = ClampSpeed(packSpeedRpm, g_washingSystem.packMotor.maxSpeed);

    g_washingSystem.lowerScrew.motor1.direction = DIR_CCW;
    g_washingSystem.lowerScrew.motor2.direction = DIR_CCW;
    g_washingSystem.lowerScrew.motor1.targetSpeed = screwRpm;
    g_washingSystem.lowerScrew.motor2.targetSpeed = screwRpm;
    g_washingSystem.lowerScrew.motor1.isEnabled = true;
    g_washingSystem.lowerScrew.motor2.isEnabled = true;
    g_washingSystem.lowerScrew.targetSpeedRpm = screwRpm;
    g_washingSystem.lowerScrew.targetPulses = LOWER_SCREW_PULSES;
    g_washingSystem.lowerScrew.isMoving = true;

    g_washingSystem.packMotor.direction = DIR_CCW;
    g_washingSystem.packMotor.targetSpeed = packRpm;
    g_washingSystem.packMotor.isEnabled = true;

    /* 先缓存 5/6 号位置命令和 7 号速度命令，再让三台电机同步启动。 */
    Emm_V5_En_Control(g_washingSystem.lowerScrew.motor1.motorID, true, true);
    HAL_Delay(50);
    Emm_V5_En_Control(g_washingSystem.lowerScrew.motor2.motorID, true, true);
    HAL_Delay(50);
    Emm_V5_En_Control(g_washingSystem.packMotor.motorID, true, true);
    HAL_Delay(50);

    Emm_V5_Pos_Control(g_washingSystem.lowerScrew.motor1.motorID,
                       DIR_CCW,
                       screwRpm,
                       LOWER_SCREW_ACCEL,
                       LOWER_SCREW_PULSES,
                       false,
                       true);
    HAL_Delay(50);
    Emm_V5_Pos_Control(g_washingSystem.lowerScrew.motor2.motorID,
                       DIR_CCW,
                       screwRpm,
                       LOWER_SCREW_ACCEL,
                       LOWER_SCREW_PULSES,
                       false,
                       true);
    HAL_Delay(50);
    Emm_V5_Vel_Control(g_washingSystem.packMotor.motorID,
                       DIR_CCW,
                       packRpm,
                       LOWER_SCREW_ACCEL,
                       true);
    HAL_Delay(50);
    Emm_V5_Synchronous_motion(BROADCAST_ADDR);

    return true;
}

/**
 * @brief 初始化电机控制系统
 * @note 电机命令统一通过 CAN2 -> Emm_V5 -> can_SendCmd 发送。
 */
void WashingSystem_Init(void)
{
    uint8_t i;
    memset(&g_washingSystem, 0, sizeof(WashingSystem));

    g_washingSystem.defaultAccel = DEFAULT_ACCEL;
    g_washingSystem.upperBelt.layerID = LAYER_UPPER;
    g_washingSystem.middleBelt.layerID = LAYER_MIDDLE;

    ConfigureMotor(&g_washingSystem.upperBelt.motor1, MOTOR_UPPER_1, MAX_SPEED_UPPER, DIR_CW);
    ConfigureMotor(&g_washingSystem.upperBelt.motor2, MOTOR_UPPER_2, MAX_SPEED_UPPER, DIR_CW);

    ConfigureMotor(&g_washingSystem.middleBelt.motor1, MOTOR_MIDDLE_1, MAX_SPEED_MIDDLE, DIR_CCW);
    ConfigureMotor(&g_washingSystem.middleBelt.motor2, MOTOR_MIDDLE_2, MAX_SPEED_MIDDLE, DIR_CW);

    ConfigureMotor(&g_washingSystem.lowerScrew.motor1, MOTOR_LOWER_SCREW_1, MAX_SPEED_LOWER_SCREW, DIR_CCW);
    ConfigureMotor(&g_washingSystem.lowerScrew.motor2, MOTOR_LOWER_SCREW_2, MAX_SPEED_LOWER_SCREW, DIR_CCW);
    g_washingSystem.lowerScrew.targetPulses = LOWER_SCREW_PULSES;

    ConfigureMotor(&g_washingSystem.packMotor, MOTOR_PACK, MAX_SPEED_PACK, DIR_CCW);

    HAL_Delay(2000);

    // 第三代只初始化 1-7 号电机；8号电机不参与控制。
    uint8_t all_motors[] = {
        MOTOR_UPPER_1, MOTOR_UPPER_2,
        MOTOR_MIDDLE_1, MOTOR_MIDDLE_2,
        MOTOR_LOWER_SCREW_1, MOTOR_LOWER_SCREW_2,
        MOTOR_PACK
    };

    for (i = 0; i < (uint8_t)(sizeof(all_motors) / sizeof(all_motors[0])); i++)
    {
        uint8_t id = all_motors[i];

        Emm_V5_Reset_Clog_Pro(id);
        HAL_Delay(150);

        /* 所有电机使用闭环控制模式；速度/位置运动由具体命令决定。 */
        Emm_V5_Modify_Ctrl_Mode(id, false, true);
        HAL_Delay(150);

        Emm_V5_En_Control(id, true, false);
        HAL_Delay(150);
    }

    for (i = 0; i < (uint8_t)(sizeof(all_motors) / sizeof(all_motors[0])); i++)
    {
        /* 对已知电机逐个清零，避免误影响仍接在总线上的 8 号电机。 */
        Emm_V5_Reset_CurPos_To_Zero(all_motors[i]);
        HAL_Delay(50);
    }

    g_washingSystem.isInitialized = true;
    g_washingSystem.emergencyStop = false;
}

/**
 * @brief 辅助函数：通过层获取上/中层传送带对象
 */
ConveyorBelt* GetConveyorBeltByLayer(LayerType layer)
{
    switch (layer)
    {
        case LAYER_UPPER:
            return &g_washingSystem.upperBelt;
        case LAYER_MIDDLE:
            return &g_washingSystem.middleBelt;
        default:
            return NULL;
    }
}

/**
 * @brief 设置并启动上/中层传送带
 */
bool StartConveyorBelt(LayerType layer, float speedRpm)
{
    if (g_washingSystem.emergencyStop)
    {
        return false;
    }

    ConveyorBelt *belt = GetConveyorBeltByLayer(layer);
    if (!belt)
    {
        return false;
    }

    uint16_t rpm1 = ClampSpeed(speedRpm, belt->motor1.maxSpeed);
    uint16_t rpm2 = ClampSpeed(speedRpm, belt->motor2.maxSpeed);

    belt->targetSpeedRpm = (float)rpm1;
    belt->motor1.targetSpeed = rpm1;
    belt->motor2.targetSpeed = rpm2;
    belt->motor1.isEnabled = true;
    belt->motor2.isEnabled = true;

    /* 上层/中层都是双电机组，同步广播可以让两侧同时起动。 */
    Emm_V5_En_Control(belt->motor1.motorID, true, true);
    HAL_Delay(50);
    Emm_V5_En_Control(belt->motor2.motorID, true, true);
    HAL_Delay(50);

    Emm_V5_Vel_Control(belt->motor1.motorID, belt->motor1.direction, rpm1, g_washingSystem.defaultAccel, true);
    HAL_Delay(50);
    Emm_V5_Vel_Control(belt->motor2.motorID, belt->motor2.direction, rpm2, g_washingSystem.defaultAccel, true);
    HAL_Delay(50);
    Emm_V5_Synchronous_motion(BROADCAST_ADDR);

    return true;
}

/**
 * @brief 停止上/中层传送带
 */
bool StopConveyorBelt(LayerType layer)
{
    ConveyorBelt *belt = GetConveyorBeltByLayer(layer);
    if (!belt)
    {
        return false;
    }

    belt->motor1.isEnabled = false;
    belt->motor2.isEnabled = false;
    belt->motor1.targetSpeed = 0;
    belt->motor2.targetSpeed = 0;
    belt->targetSpeedRpm = 0.0f;

    Emm_V5_Stop_Now(belt->motor1.motorID, false);
    HAL_Delay(50);
    Emm_V5_Stop_Now(belt->motor2.motorID, false);
    HAL_Delay(50);

    return true;
}

/**
 * @brief 5/6号丝杆推板推动：CCW 位置模式 240000 脉冲
 */
bool StartLowerPush(float speedRpm)
{
    return StartLowerScrewMove(DIR_CCW, speedRpm);
}

/**
 * @brief 5/6号推板和7号打包电机同步启动
 */
bool StartLowerPushAndPack(float screwSpeedRpm, float packSpeedRpm)
{
    return StartLowerPushAndPackMove(screwSpeedRpm, packSpeedRpm);
}

/**
 * @brief 5/6号丝杆推板返回：CW 位置模式 240000 脉冲
 */
bool StartLowerReturn(float speedRpm)
{
    /* 返回使用相同的相对脉冲数，只是方向与推动相反。 */
    return StartLowerScrewMove(DIR_CW, speedRpm);
}

bool StopLowerScrew(void)
{
    g_washingSystem.lowerScrew.motor1.isEnabled = false;
    g_washingSystem.lowerScrew.motor2.isEnabled = false;
    g_washingSystem.lowerScrew.motor1.targetSpeed = 0;
    g_washingSystem.lowerScrew.motor2.targetSpeed = 0;
    g_washingSystem.lowerScrew.isMoving = false;

    Emm_V5_Stop_Now(g_washingSystem.lowerScrew.motor1.motorID, false);
    HAL_Delay(50);
    Emm_V5_Stop_Now(g_washingSystem.lowerScrew.motor2.motorID, false);
    HAL_Delay(50);

    return true;
}

bool StartPackMotor(float speedRpm)
{
    if (g_washingSystem.emergencyStop)
    {
        return false;
    }

    uint16_t rpm = ClampSpeed(speedRpm, g_washingSystem.packMotor.maxSpeed);
    g_washingSystem.packMotor.targetSpeed = rpm;
    g_washingSystem.packMotor.isEnabled = true;
    g_washingSystem.packMotor.direction = DIR_CCW;

    Emm_V5_En_Control(g_washingSystem.packMotor.motorID, true, false);
    HAL_Delay(50);
    Emm_V5_Vel_Control(g_washingSystem.packMotor.motorID,
                       g_washingSystem.packMotor.direction,
                       rpm,
                       LOWER_SCREW_ACCEL,
                       false);
    HAL_Delay(50);

    return true;
}

bool StopPackMotor(void)
{
    g_washingSystem.packMotor.isEnabled = false;
    g_washingSystem.packMotor.targetSpeed = 0;

    Emm_V5_Stop_Now(g_washingSystem.packMotor.motorID, false);
    HAL_Delay(50);

    return true;
}

/**
 * @brief 全局急停
 */
void EmergencyStopAll(void)
{
    g_washingSystem.emergencyStop = true;

    // 广播停止会影响总线上全部电机；第三代软件只维护 1-7 号状态。
    Emm_V5_Stop_Now(BROADCAST_ADDR, false);
    HAL_Delay(50);
    Emm_V5_En_Control(BROADCAST_ADDR, false, false);
    HAL_Delay(50);

    g_washingSystem.upperBelt.motor1.isEnabled = false;
    g_washingSystem.upperBelt.motor2.isEnabled = false;
    g_washingSystem.middleBelt.motor1.isEnabled = false;
    g_washingSystem.middleBelt.motor2.isEnabled = false;
    g_washingSystem.lowerScrew.motor1.isEnabled = false;
    g_washingSystem.lowerScrew.motor2.isEnabled = false;
    g_washingSystem.lowerScrew.isMoving = false;
    g_washingSystem.packMotor.isEnabled = false;
}

/**
 * @brief 复位急停
 */
bool ResetEmergencyStop(void)
{
    if (g_washingSystem.emergencyStop)
    {
        Emm_V5_Reset_Clog_Pro(BROADCAST_ADDR);
        HAL_Delay(50);
        g_washingSystem.emergencyStop = false;
    }
    return true;
}

/**
 * @brief 停止整个洗菜机系统
 */
void StopWashingSystem(void)
{
    StopConveyorBelt(LAYER_UPPER);
    StopConveyorBelt(LAYER_MIDDLE);
    StopLowerScrew();
    StopPackMotor();
}
