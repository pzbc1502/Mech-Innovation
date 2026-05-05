#ifndef __CONVEYOR_BELT_H
#define __CONVEYOR_BELT_H

#include "Emm_V5.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * Third-generation motor link:
 * CAN2 -> Emm_V5 -> can_SendCmd.
 * Only motor IDs 1-7 are scheduled. Motor 8 is intentionally unused.
 */

/* Motor IDs. */
#define MOTOR_UPPER_1          1
#define MOTOR_UPPER_2          2
#define MOTOR_MIDDLE_1         3
#define MOTOR_MIDDLE_2         4
#define MOTOR_LOWER_SCREW_1    5
#define MOTOR_LOWER_SCREW_2    6
#define MOTOR_PACK             7
#define BROADCAST_ADDR         0x00

/* Third-generation process keeps only upper and middle conveyor layers. */
typedef enum {
    LAYER_UPPER = 1,
    LAYER_MIDDLE = 2
} LayerType;

/* Motor direction. */
#define DIR_CW    0
#define DIR_CCW   1

/* Speed limits, unit: RPM. */
#define MAX_SPEED_UPPER        2000
#define MAX_SPEED_MIDDLE       2000
#define MAX_SPEED_LOWER_SCREW  1000
#define MAX_SPEED_PACK         1000

#define DEFAULT_ACCEL          20
#define LOWER_SCREW_ACCEL      0
#define PACK_MOTOR_SPEED_RPM   30U
#define PACK_MOTOR_ACCEL       5U

/*
 * Screw push distance is 300 mm.
 * The current configured position target is 240000 pulses.
 */
#define LOWER_SCREW_PULSES     105000UL

typedef struct {
    uint8_t motorID;
    uint8_t direction;
    bool isEnabled;
    uint16_t targetSpeed;
    uint16_t actualSpeed;
    uint16_t maxSpeed;
    bool isStalled;
} MotorControl;

/* A conveyor layer is driven by two synchronized motors. */
typedef struct {
    LayerType layerID;
    MotorControl motor1;
    MotorControl motor2;
    float targetSpeedRpm;
} ConveyorBelt;

/* Lower screw push plate: motors 5/6, position mode, synchronized by broadcast start. */
typedef struct {
    MotorControl motor1;
    MotorControl motor2;
    uint32_t targetPulses;
    uint16_t targetSpeedRpm;
    bool isMoving;
} LowerScrewPush;

typedef struct {
    ConveyorBelt upperBelt;
    ConveyorBelt middleBelt;
    LowerScrewPush lowerScrew;
    MotorControl packMotor;
    uint8_t defaultAccel;
    bool isInitialized;
    bool emergencyStop;
} WashingSystem;

extern WashingSystem g_washingSystem;

void WashingSystem_Init(void);

bool StartConveyorBelt(LayerType layer, float speedRpm);
bool StopConveyorBelt(LayerType layer);

bool StartLowerPush(float speedRpm);
bool StartLowerPushAndPack(float screwSpeedRpm, float packSpeedRpm);
bool StartLowerReturn(float speedRpm);
bool StopLowerScrew(void);

bool StartPackMotor(float speedRpm);
bool StopPackMotor(void);

void EmergencyStopAll(void);
bool ResetEmergencyStop(void);
void StopWashingSystem(void);

ConveyorBelt* GetConveyorBeltByLayer(LayerType layer);

#endif /* __CONVEYOR_BELT_H */
