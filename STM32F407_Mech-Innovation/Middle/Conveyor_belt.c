#include "Conveyor_belt.h"

#include <string.h>

WashingSystem g_washingSystem;

static void DelayAfterCanCommand(void)
{
    HAL_Delay(50);
}

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

static void EnableMotorNow(uint8_t motorId)
{
    /*
     * Enable immediately. Only motion commands use snF=true, then one
     * broadcast sync command starts the prepared motors together.
     */
    Emm_V5_En_Control(motorId, true, false);
    DelayAfterCanCommand();
}

static void UpdateLowerScrewState(uint8_t direction, uint16_t rpm)
{
    g_washingSystem.lowerScrew.motor1.direction = direction;
    g_washingSystem.lowerScrew.motor2.direction = direction;
    g_washingSystem.lowerScrew.motor1.targetSpeed = rpm;
    g_washingSystem.lowerScrew.motor2.targetSpeed = rpm;
    g_washingSystem.lowerScrew.motor1.isEnabled = true;
    g_washingSystem.lowerScrew.motor2.isEnabled = true;
    g_washingSystem.lowerScrew.targetSpeedRpm = rpm;
    g_washingSystem.lowerScrew.targetPulses = LOWER_SCREW_PULSES;
    g_washingSystem.lowerScrew.isMoving = true;
}

static bool StartLowerScrewMove(uint8_t direction, float speedRpm)
{
    uint16_t rpm;

    if (g_washingSystem.emergencyStop)
    {
        return false;
    }

    rpm = ClampSpeed(speedRpm, MAX_SPEED_LOWER_SCREW);
    UpdateLowerScrewState(direction, rpm);

    EnableMotorNow(g_washingSystem.lowerScrew.motor1.motorID);
    EnableMotorNow(g_washingSystem.lowerScrew.motor2.motorID);

    Emm_V5_Pos_Control(g_washingSystem.lowerScrew.motor1.motorID,
                       direction,
                       rpm,
                       LOWER_SCREW_ACCEL,
                       LOWER_SCREW_PULSES,
                       false,
                       true);
    DelayAfterCanCommand();

    Emm_V5_Pos_Control(g_washingSystem.lowerScrew.motor2.motorID,
                       direction,
                       rpm,
                       LOWER_SCREW_ACCEL,
                       LOWER_SCREW_PULSES,
                       false,
                       true);
    DelayAfterCanCommand();

    Emm_V5_Synchronous_motion(BROADCAST_ADDR);

    return true;
}

static bool StartLowerPushAndPackMove(float screwSpeedRpm, float packSpeedRpm)
{
    uint16_t screwRpm;
    uint16_t packRpm;

    if (g_washingSystem.emergencyStop)
    {
        return false;
    }

    screwRpm = ClampSpeed(screwSpeedRpm, MAX_SPEED_LOWER_SCREW);
    packRpm = ClampSpeed(packSpeedRpm, g_washingSystem.packMotor.maxSpeed);

    UpdateLowerScrewState(DIR_CCW, screwRpm);

    g_washingSystem.packMotor.direction = DIR_CCW;
    g_washingSystem.packMotor.targetSpeed = packRpm;
    g_washingSystem.packMotor.isEnabled = true;

    EnableMotorNow(g_washingSystem.lowerScrew.motor1.motorID);
    EnableMotorNow(g_washingSystem.lowerScrew.motor2.motorID);
    EnableMotorNow(g_washingSystem.packMotor.motorID);

    Emm_V5_Pos_Control(g_washingSystem.lowerScrew.motor1.motorID,
                       DIR_CCW,
                       screwRpm,
                       LOWER_SCREW_ACCEL,
                       LOWER_SCREW_PULSES,
                       false,
                       true);
    DelayAfterCanCommand();

    Emm_V5_Pos_Control(g_washingSystem.lowerScrew.motor2.motorID,
                       DIR_CCW,
                       screwRpm,
                       LOWER_SCREW_ACCEL,
                       LOWER_SCREW_PULSES,
                       false,
                       true);
    DelayAfterCanCommand();

    Emm_V5_Vel_Control(g_washingSystem.packMotor.motorID,
                       DIR_CCW,
                       packRpm,
                       LOWER_SCREW_ACCEL,
                       true);
    DelayAfterCanCommand();

    Emm_V5_Synchronous_motion(BROADCAST_ADDR);

    return true;
}

void WashingSystem_Init(void)
{
    uint8_t i;
    uint8_t allMotors[] = {
        MOTOR_UPPER_1,
        MOTOR_UPPER_2,
        MOTOR_MIDDLE_1,
        MOTOR_MIDDLE_2,
        MOTOR_LOWER_SCREW_1,
        MOTOR_LOWER_SCREW_2,
        MOTOR_PACK
    };

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

    for (i = 0; i < (uint8_t)(sizeof(allMotors) / sizeof(allMotors[0])); i++)
    {
        uint8_t id = allMotors[i];

        Emm_V5_Reset_Clog_Pro(id);
        HAL_Delay(150);

        Emm_V5_Modify_Ctrl_Mode(id, false, true);
        HAL_Delay(150);

        Emm_V5_En_Control(id, true, false);
        HAL_Delay(150);
    }

    for (i = 0; i < (uint8_t)(sizeof(allMotors) / sizeof(allMotors[0])); i++)
    {
        Emm_V5_Reset_CurPos_To_Zero(allMotors[i]);
        DelayAfterCanCommand();
    }

    g_washingSystem.isInitialized = true;
    g_washingSystem.emergencyStop = false;
}

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

bool StartConveyorBelt(LayerType layer, float speedRpm)
{
    ConveyorBelt *belt;
    uint16_t rpm1;
    uint16_t rpm2;

    if (g_washingSystem.emergencyStop)
    {
        return false;
    }

    belt = GetConveyorBeltByLayer(layer);
    if (belt == NULL)
    {
        return false;
    }

    rpm1 = ClampSpeed(speedRpm, belt->motor1.maxSpeed);
    rpm2 = ClampSpeed(speedRpm, belt->motor2.maxSpeed);

    belt->targetSpeedRpm = (float)rpm1;
    belt->motor1.targetSpeed = rpm1;
    belt->motor2.targetSpeed = rpm2;
    belt->motor1.isEnabled = true;
    belt->motor2.isEnabled = true;

    EnableMotorNow(belt->motor1.motorID);
    EnableMotorNow(belt->motor2.motorID);

    Emm_V5_Vel_Control(belt->motor1.motorID,
                       belt->motor1.direction,
                       rpm1,
                       g_washingSystem.defaultAccel,
                       true);
    DelayAfterCanCommand();

    Emm_V5_Vel_Control(belt->motor2.motorID,
                       belt->motor2.direction,
                       rpm2,
                       g_washingSystem.defaultAccel,
                       true);
    DelayAfterCanCommand();

    Emm_V5_Synchronous_motion(BROADCAST_ADDR);

    return true;
}

bool StopConveyorBelt(LayerType layer)
{
    ConveyorBelt *belt;

    belt = GetConveyorBeltByLayer(layer);
    if (belt == NULL)
    {
        return false;
    }

    belt->motor1.isEnabled = false;
    belt->motor2.isEnabled = false;
    belt->motor1.targetSpeed = 0;
    belt->motor2.targetSpeed = 0;
    belt->targetSpeedRpm = 0.0f;

    Emm_V5_Stop_Now(belt->motor1.motorID, false);
    DelayAfterCanCommand();
    Emm_V5_Stop_Now(belt->motor2.motorID, false);
    DelayAfterCanCommand();

    return true;
}

bool StartLowerPush(float speedRpm)
{
    return StartLowerScrewMove(DIR_CCW, speedRpm);
}

bool StartLowerPushAndPack(float screwSpeedRpm, float packSpeedRpm)
{
    return StartLowerPushAndPackMove(screwSpeedRpm, packSpeedRpm);
}

bool StartLowerReturn(float speedRpm)
{
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
    DelayAfterCanCommand();
    Emm_V5_Stop_Now(g_washingSystem.lowerScrew.motor2.motorID, false);
    DelayAfterCanCommand();

    return true;
}

bool StartPackMotor(float speedRpm)
{
    uint16_t rpm;

    if (g_washingSystem.emergencyStop)
    {
        return false;
    }

    rpm = ClampSpeed(speedRpm, g_washingSystem.packMotor.maxSpeed);
    g_washingSystem.packMotor.targetSpeed = rpm;
    g_washingSystem.packMotor.isEnabled = true;
    g_washingSystem.packMotor.direction = DIR_CCW;

    EnableMotorNow(g_washingSystem.packMotor.motorID);

    Emm_V5_Vel_Control(g_washingSystem.packMotor.motorID,
                       g_washingSystem.packMotor.direction,
                       rpm,
                       LOWER_SCREW_ACCEL,
                       false);
    DelayAfterCanCommand();

    return true;
}

bool StopPackMotor(void)
{
    g_washingSystem.packMotor.isEnabled = false;
    g_washingSystem.packMotor.targetSpeed = 0;

    Emm_V5_Stop_Now(g_washingSystem.packMotor.motorID, false);
    DelayAfterCanCommand();

    return true;
}

void EmergencyStopAll(void)
{
    g_washingSystem.emergencyStop = true;

    Emm_V5_Stop_Now(BROADCAST_ADDR, false);
    DelayAfterCanCommand();
    Emm_V5_En_Control(BROADCAST_ADDR, false, false);
    DelayAfterCanCommand();

    g_washingSystem.upperBelt.motor1.isEnabled = false;
    g_washingSystem.upperBelt.motor2.isEnabled = false;
    g_washingSystem.middleBelt.motor1.isEnabled = false;
    g_washingSystem.middleBelt.motor2.isEnabled = false;
    g_washingSystem.lowerScrew.motor1.isEnabled = false;
    g_washingSystem.lowerScrew.motor2.isEnabled = false;
    g_washingSystem.lowerScrew.isMoving = false;
    g_washingSystem.packMotor.isEnabled = false;
}

bool ResetEmergencyStop(void)
{
    if (g_washingSystem.emergencyStop)
    {
        Emm_V5_Reset_Clog_Pro(BROADCAST_ADDR);
        DelayAfterCanCommand();
        g_washingSystem.emergencyStop = false;
    }

    return true;
}

void StopWashingSystem(void)
{
    StopConveyorBelt(LAYER_UPPER);
    StopConveyorBelt(LAYER_MIDDLE);
    StopLowerScrew();
    StopPackMotor();
}
