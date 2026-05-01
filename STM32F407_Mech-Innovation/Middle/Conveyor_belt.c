#include "Conveyor_belt.h"

// 全局系统结构体
WashingSystem g_washingSystem;

/**
 * @brief 初始化电机控制系统
 * @note Motor commands use CAN2 through Emm_V5/can_SendCmd.
 * 核心修改：先配置软件参数，再发送硬件指令，确保逻辑一致性
 */
void WashingSystem_Init(void) {
    // 1. 初始化软件结构体参数 (先软件)
    memset(&g_washingSystem, 0, sizeof(WashingSystem));
    
    // 配置默认参数
    g_washingSystem.defaultAccel = DEFAULT_ACCEL;
    g_washingSystem.upperBelt.layerID = LAYER_UPPER;
    g_washingSystem.middleBelt.layerID = LAYER_MIDDLE;
    g_washingSystem.lowerBelt.layerID = LAYER_LOWER;
    
    // 配置上层电机参数
    g_washingSystem.upperBelt.motor1.motorID = MOTOR_UPPER_1;
    g_washingSystem.upperBelt.motor2.motorID = MOTOR_UPPER_2;
    g_washingSystem.upperBelt.motor1.maxSpeed = MAX_SPEED_UPPER;
    g_washingSystem.upperBelt.motor2.maxSpeed = MAX_SPEED_UPPER;
    // 方向统一配置，如果硬件接反了，在这里改 DIR_CW 即可
    g_washingSystem.upperBelt.motor1.direction = DIR_CW; 
    g_washingSystem.upperBelt.motor2.direction = DIR_CW;
     
    // 配置中层电机
    g_washingSystem.middleBelt.motor1.motorID = MOTOR_MIDDLE_1;
    g_washingSystem.middleBelt.motor2.motorID = MOTOR_MIDDLE_2;
    g_washingSystem.middleBelt.motor1.maxSpeed = MAX_SPEED_MIDDLE;
    g_washingSystem.middleBelt.motor2.maxSpeed = MAX_SPEED_MIDDLE;
    g_washingSystem.middleBelt.motor1.direction = DIR_CCW;   //左边
    g_washingSystem.middleBelt.motor2.direction = DIR_CW;   //右边
     
    // 配置下层电机
    g_washingSystem.lowerBelt.motor1.motorID = MOTOR_LOWER_1;
    g_washingSystem.lowerBelt.motor2.motorID = MOTOR_LOWER_2;
    g_washingSystem.lowerBelt.motor1.maxSpeed = MAX_SPEED_LOWER;
    g_washingSystem.lowerBelt.motor2.maxSpeed = MAX_SPEED_LOWER;
    g_washingSystem.lowerBelt.motor1.direction = DIR_CW;  
    g_washingSystem.lowerBelt.motor2.direction = DIR_CCW;

    // 配置打包机构电机
    g_washingSystem.packMotor1.motorID = MOTOR_PACK_1;
    g_washingSystem.packMotor1.maxSpeed = MAX_SPEED_PACK_1;
    g_washingSystem.packMotor1.direction = DIR_CW;

    g_washingSystem.packMotor2.motorID = MOTOR_PACK_2;
    g_washingSystem.packMotor2.maxSpeed = MAX_SPEED_PACK_2;
    g_washingSystem.packMotor2.direction = DIR_CCW;

	HAL_Delay(2000);
	
    // 2. 发送硬件初始化指令 (后硬件)
    // 遍历所有定义的电机ID进行复位
    uint8_t all_motors[] = {
        MOTOR_UPPER_1, MOTOR_UPPER_2,
        MOTOR_MIDDLE_1, MOTOR_MIDDLE_2,
        MOTOR_LOWER_1, MOTOR_LOWER_2,
        MOTOR_PACK_1, MOTOR_PACK_2
    };

    for (uint8_t i = 0; i < (uint8_t)(sizeof(all_motors) / sizeof(all_motors[0])); i++)
    {
        uint8_t id = all_motors[i];

        // 重置堵转保护
        Emm_V5_Reset_Clog_Pro(id);
        HAL_Delay(150);

        // 设置控制模式
        Emm_V5_Modify_Ctrl_Mode(id, false, true);
        HAL_Delay(150);

        // 启用控制
        Emm_V5_En_Control(id, true, false);
        HAL_Delay(150);
    }
	Emm_V5_Reset_CurPos_To_Zero(BROADCAST_ADDR);
	
    
    g_washingSystem.isInitialized = true;
    g_washingSystem.emergencyStop = false;
}

/**
 * @brief 辅助函数：通过层获取对象
 */
ConveyorBelt* GetConveyorBeltByLayer(LayerType layer) {
    switch(layer) {
        case LAYER_UPPER:  return &g_washingSystem.upperBelt;
        case LAYER_MIDDLE: return &g_washingSystem.middleBelt;
        case LAYER_LOWER:  return &g_washingSystem.lowerBelt;
        default: return NULL;
    }
}

/**
 * @brief 设置并启动传送带
 * 优化点：整合了 Enable 和 Speed 设置，减少通信次数
 */
bool StartConveyorBelt(LayerType layer, float speedRpm) {
    if (g_washingSystem.emergencyStop) return false;
    
    ConveyorBelt* belt = GetConveyorBeltByLayer(layer);
    if (!belt) return false;

    // 1. 更新结构体状态
    if (speedRpm < 0.0f) speedRpm = 0.0f;
    belt->targetSpeedRpm = speedRpm;

    // 直接使用传入速度值(RPM)
    uint16_t rpm1 = (uint16_t)(speedRpm);
    uint16_t rpm2 = (uint16_t)(speedRpm);
    
    belt->motor1.targetSpeed = rpm1;
    belt->motor2.targetSpeed = rpm2;
    belt->motor1.isEnabled = true;
    belt->motor2.isEnabled = true;

    // 2. 硬件控制 - 关键优化：先发参数，最后同步
    // 使用带 Sync=true 的指令，这样电机收到指令后暂不执行，等待广播包

    // 电机1 使能 (Pending) - 其实Vel Control会自动启动，这步可以省略，但为了保险
    Emm_V5_En_Control(belt->motor1.motorID, true, true);
	HAL_Delay(50);
    // 电机2 使能 (Pending)
    Emm_V5_En_Control(belt->motor2.motorID, true, true);
	HAL_Delay(50);
	
    // 电机1 设置速度 (Pending)
    Emm_V5_Vel_Control(belt->motor1.motorID, belt->motor1.direction, rpm1, g_washingSystem.defaultAccel, true);
	HAL_Delay(50);
    // 电机2 设置速度 (Pending)
    Emm_V5_Vel_Control(belt->motor2.motorID, belt->motor2.direction, rpm2, g_washingSystem.defaultAccel, true);
	HAL_Delay(50);
    

    // 3. 发送广播同步包，所有 Pending 的电机同时动作
    // 0x00 表示广播地址(根据你的Emm_V5.c里的实现，传0会发00，有些协议是FF，请确认Emm_V5实现)
    Emm_V5_Synchronous_motion(BROADCAST_ADDR); 

    return true;
}

/**
 * @brief 停止传送带
 */
bool StopConveyorBelt(LayerType layer) {
    ConveyorBelt* belt = GetConveyorBeltByLayer(layer);
    if (!belt) return false;
    
    // 更新状态
    belt->motor1.isEnabled = false;
    belt->motor2.isEnabled = false;
    belt->targetSpeedRpm = 0;
    
    // 立即停止，不使用同步，防止危险
    Emm_V5_Stop_Now(belt->motor1.motorID, false);
	HAL_Delay(50);
    Emm_V5_Stop_Now(belt->motor2.motorID, false);
	HAL_Delay(50);
    
    return true;
}

bool StartPackConveyor(float speedRpm)
{
    if (g_washingSystem.emergencyStop) return false;

    if(speedRpm < 0.0f) speedRpm = 0.0f;

    uint16_t rpm = (uint16_t)(speedRpm);
    g_washingSystem.packMotor1.targetSpeed = rpm;
    g_washingSystem.packMotor1.isEnabled = true;

    Emm_V5_En_Control(g_washingSystem.packMotor1.motorID, true, false);
    HAL_Delay(50);
    Emm_V5_Vel_Control(g_washingSystem.packMotor1.motorID, g_washingSystem.packMotor1.direction, rpm, g_washingSystem.defaultAccel, false);
    HAL_Delay(50);

    return true;
}

bool StopPackConveyor(void)
{
    g_washingSystem.packMotor1.isEnabled = false;
    g_washingSystem.packMotor1.targetSpeed = 0;

    Emm_V5_Stop_Now(g_washingSystem.packMotor1.motorID, false);
    HAL_Delay(50);

    return true;
}

bool StartWrapMotor(float speedRpm)
{
    if (g_washingSystem.emergencyStop) return false;

    if(speedRpm < 0.0f) speedRpm = 0.0f;

    uint16_t rpm = (uint16_t)(speedRpm);
    g_washingSystem.packMotor2.targetSpeed = rpm;
    g_washingSystem.packMotor2.isEnabled = true;

    Emm_V5_En_Control(g_washingSystem.packMotor2.motorID, true, false);
    HAL_Delay(50);
    Emm_V5_Vel_Control(g_washingSystem.packMotor2.motorID, g_washingSystem.packMotor2.direction, rpm, g_washingSystem.defaultAccel, false);
    HAL_Delay(50);

    return true;
}

bool StopWrapMotor(void)
{
    g_washingSystem.packMotor2.isEnabled = false;
    g_washingSystem.packMotor2.targetSpeed = 0;

    Emm_V5_Stop_Now(g_washingSystem.packMotor2.motorID, false);
    HAL_Delay(50);

    return true;
}

/**
 * @brief 全局急停
 */
void EmergencyStopAll(void) {
    g_washingSystem.emergencyStop = true;
    
    // 广播停止
    Emm_V5_Stop_Now(BROADCAST_ADDR, false);
	HAL_Delay(50);
    
    // 也要禁用使能，防止误触
    Emm_V5_En_Control(BROADCAST_ADDR, false, false);
	HAL_Delay(50);
    
    // 更新所有软件状态
    g_washingSystem.upperBelt.motor1.isEnabled = false;
    g_washingSystem.upperBelt.motor2.isEnabled = false;
    g_washingSystem.middleBelt.motor1.isEnabled = false;
    g_washingSystem.middleBelt.motor2.isEnabled = false;
    g_washingSystem.lowerBelt.motor1.isEnabled = false;
    g_washingSystem.lowerBelt.motor2.isEnabled = false;
    g_washingSystem.packMotor1.isEnabled = false;
    g_washingSystem.packMotor2.isEnabled = false;
}

/**
 * @brief 复位急停
 */
bool ResetEmergencyStop(void) {
    // 必须确保急停开关已物理复位（如果是硬件急停），这里只处理软件逻辑
    if (g_washingSystem.emergencyStop) {
        // 解除所有电机的堵转/错误状态
        Emm_V5_Reset_Clog_Pro(BROADCAST_ADDR);
        HAL_Delay(50);
        
        g_washingSystem.emergencyStop = false;
    }
    return true;
}


/**
 * @brief 停止整个洗菜机系统 (用于非急停的普通停止)
 *        依次停止三层传送带
 */
void StopWashingSystem(void) {
    // 调用已有的单层停止函数
    StopConveyorBelt(LAYER_UPPER);
    StopConveyorBelt(LAYER_MIDDLE);
    StopConveyorBelt(LAYER_LOWER);
    StopPackConveyor();
    StopWrapMotor();
}




