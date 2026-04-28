#include "app_Conveyor.h"
#include "debug.h" 
#include "bsp_bluetooth.h"
#include "bsp_relay.h"
#include "HX711.h"
#include "Servo_Ctrl.h"

// 定义全局应用控制句柄
static App_WashingCtrl_t appCtrl;

// 声明内部辅助函数
static void Run_Auto_Process_FSM(void);
static void Run_Maintenance_Mode(void);

/**
 * @brief 应用层初始化
 */
void App_Conwashing_Init(void) {
    // 1. 初始化底层硬件和中间层
    WashingSystem_Init();
    
    // 2. 初始化应用层状态
    memset(&appCtrl, 0, sizeof(App_WashingCtrl_t));
    appCtrl.currentMode = MODE_IDLE;
    appCtrl.autoState = STEP_READY;
    
    // 3. 加载默认参数
    appCtrl.config.upperSpeed = 130.0f;  // 上层速度
    appCtrl.config.middleSpeed = 130.0f; // 中间速度
    appCtrl.config.lowerSpeed = 130.0f;  // 下层速度
    appCtrl.config.packConveyorSpeed = 100.0f; // 7号电机速度
    appCtrl.config.packWrapSpeed = 100.0f;     // 8号电机速度
     
    appCtrl.config.runDuration = 5000; // 传送带到位时间
    appCtrl.config.runDuration_Low = 3000; // 下层传送带走3秒到位
    appCtrl.config.packMotor7TimeMs = 4000; // 7号电机运行时间
    appCtrl.config.packMotor8TimeMs = 9000; // 8号电机运行时间
    appCtrl.config.washTime    = 100; // 冲洗时间
    appCtrl.config.weighTime   = 200; // 称重等待2秒
    appCtrl.config.cutTime     = 200; // 切刀动作1秒
    appCtrl.config.servoTime   = 200; // 切刀动作1秒


    // 调试打印
    #ifdef DEBUG_ENABLE
    printf("[APP] Washing System Initialized\r\n");
    #endif
    Bluetooth_SendString("[APP] Washing System Initialized\r\n");
}


/**
 * @brief 应用层主任务，处理状态机
 * @note  建议每10ms调用一次
 * @note  在主函数的while循环里调用
 */
void App_Conwashing_Task(void) 
{
    // 状态机计时器累加 (调用周期)
    appCtrl.stateTimer += 10; 
      
    // 1. 紧急停止高优先级检查
    if (g_washingSystem.emergencyStop) 
		{
        if (appCtrl.currentMode != MODE_ERROR) 
				{
            appCtrl.currentMode = MODE_ERROR;
            #ifdef DEBUG_ENABLE
            printf("[APP] EMERGENCY STOP TRIGGERED!\r\n");
            #endif
						Bluetooth_SendString("[APP] EMERGENCY STOP TRIGGERED!\r\n");
        }
        return; // 急停状态下不执行其他逻辑
    }

    // 2. 主状态机
    switch (appCtrl.currentMode) 
	{
        case MODE_IDLE:
            // 空闲状态，什么都不做，等待 Start 指令
            break;

        case MODE_AUTO_CLEAN:
            Run_Auto_Process_FSM();
            break;

        case MODE_MAINTENANCE:
            Run_Maintenance_Mode();
            break;

        case MODE_ERROR:
            // 错误状态等待 Resume 指令
            break;
            
        default:
            break;
    }
}

/**
 * @brief 自动清洗流程的状态机 (核心逻辑)
 */
static void Run_Auto_Process_FSM(void) {
    if (appCtrl.isPaused) return;

    switch (appCtrl.autoState) {
        // --- 启动阶段 (顺序：上->中->下) ---
        case STEP_READY:
			
//			Set_Relay_Switch(RELAY_CUTTER, 0); // 刀抬起
//            Set_Relay_Switch(RELAY_PUMP, 0);   // 泵关闭
            appCtrl.stateTimer = 0;
            appCtrl.autoState = STEP_UPPER_RUN; 		
            #ifdef DEBUG_ENABLE
            printf("[APP] Auto Process Start...\r\n");
            #endif
			Bluetooth_SendString("[APP] Auto Process Start...\r\n");
            break;
		
		
		// ==========================================
        // 第一阶段：上层 (清洗)
        // ==========================================
					
        case STEP_UPPER_RUN: // 
            
            // 1. 刚进入状态：同时启动传送带和水泵
            if(appCtrl.stateTimer <= 10) {
                // 启动传送带
                StartConveyorBelt(LAYER_UPPER, appCtrl.config.upperSpeed);
                // 启动水泵
                Set_Relay_Switch(RELAY_PUMP, 1); 
                
                #ifdef DEBUG_ENABLE
                printf("[APP] Upper Process: Belt Running & Pump ON\r\n");
                #endif
                Bluetooth_SendString("[APP] Washing Start...\r\n");
            }
            
            // 2. 运行指定时间 (清洗时间 = 运行时间)
            // 这里使用 runDuration 作为总时间，washTime 参数可以不用了，或者取两者的最大值
            if (appCtrl.stateTimer > appCtrl.config.runDuration) {
                // 3. 时间到：同时停止
                StopConveyorBelt(LAYER_UPPER);
                Set_Relay_Switch(RELAY_PUMP, 0); // 关水泵
                
                #ifdef DEBUG_ENABLE
                printf("[APP] Upper Process Done.\r\n");
                #endif
                Bluetooth_SendString("[APP] Washing Done.\r\n");
                
                // 4. 跳转到下一阶段 (中层)
                appCtrl.stateTimer = 0;
                appCtrl.autoState = STEP_MIDDLE_RUN;
            }
            break;		
										
        // ==========================================
        // 第二阶段：中层 (切割)
        // ==========================================

            
        case STEP_MIDDLE_RUN: // 这里合并了运行和切割
            
            // 1. 刚进入：同时启动传送带和切刀
            if(appCtrl.stateTimer <= 10) {
                // 启动传送带
                StartConveyorBelt(LAYER_MIDDLE, appCtrl.config.middleSpeed);
				Lower_Layer_Open();
				HAL_Delay(500);
                // 启动切刀 (下刀/开始切割)
                Set_Relay_Switch(RELAY_CUTTER, 1); 
                
                #ifdef DEBUG_ENABLE
                printf("[APP] Middle Process: Belt Running & Cutter ON\r\n");
                #endif
                Bluetooth_SendString("[APP] Cutting Start...\r\n"); 
            }
            
            // 2. 运行指定时间 (切割时间 = 运行时间)
            if (appCtrl.stateTimer > appCtrl.config.runDuration) {
                // 3. 时间到：同时停止
                StopConveyorBelt(LAYER_MIDDLE);
                Set_Relay_Switch(RELAY_CUTTER, 0); // 抬刀/停止切割
                Lower_Layer_Close();
                #ifdef DEBUG_ENABLE 
                printf("[APP] Middle Process Done.\r\n");
                #endif
                Bluetooth_SendString("[APP] Cutting Done.\r\n");
                
                // 4. 跳转到下一阶段 (下层)
                appCtrl.stateTimer = 0;
                appCtrl.autoState = STEP_LOWER_RUN_1;
            }
            break;

        // ==========================================
        // 第三阶段：下层 (称重)
        // ==========================================
		
		// 1. 启动最下层
        case STEP_LOWER_RUN_1:
			
            if(appCtrl.stateTimer <= 10) {
                StartConveyorBelt(LAYER_LOWER, appCtrl.config.lowerSpeed);
                #ifdef DEBUG_ENABLE
                printf("[APP] Lower Belt Running action 1...\r\n");
                #endif
				Bluetooth_SendString("[APP] Lower Belt Running action 1...\r\n");
            }
            // 2. 运行指定时间到位
            if (appCtrl.stateTimer > appCtrl.config.runDuration_Low) {
                StopConveyorBelt(LAYER_LOWER);
                appCtrl.stateTimer = 0;
                appCtrl.autoState = STEP_LOWER_WEIGH;  
            }
            break;
			
		// 2. 称重
        case STEP_LOWER_WEIGH:
            if (appCtrl.stateTimer > appCtrl.config.weighTime) {
								Get_Maopi();
                float weight = Get_Weight(); // 读取 HX711
                #ifdef DEBUG_ENABLE
                printf("[APP] Weighing Done. Result: %.2f g\r\n", weight);
                #endif
                Bluetooth_Printf("[APP] Weighing Done. Result: %.2f g\r\n", weight);
				
								OLED_ShowString(0, 48, "weight:", OLED_8X16);
								OLED_ShowNum(60, 48, weight, 4,OLED_8X16);
                // 称重完成，进入下一层
                appCtrl.stateTimer = 0;
                appCtrl.autoState = STEP_LOWER_RUN_2;
            }
            break;			
 
        case STEP_LOWER_RUN_2:
		
            if(appCtrl.stateTimer <= 10) {
                StartConveyorBelt(LAYER_LOWER, appCtrl.config.lowerSpeed);
                #ifdef DEBUG_ENABLE
                printf("[APP] Lower Belt Running action 2...\r\n");
                #endif
				Bluetooth_SendString("[APP] Lower Belt Running action 2...\r\n");
            }
            // 2. 运行指定时间到位
            if (appCtrl.stateTimer > appCtrl.config.runDuration_Low) {
                StopConveyorBelt(LAYER_LOWER);
                appCtrl.stateTimer = 0;
                appCtrl.autoState = STEP_PACKING;
            }
            break;			

        case STEP_PACKING:
            if(appCtrl.stateTimer <= 10) {
                StartPackConveyor(appCtrl.config.packConveyorSpeed);
                #ifdef DEBUG_ENABLE
                printf("[APP] Packing Start: Motor7 Running...\r\n");
                #endif
                Bluetooth_SendString("[APP] Packing Start: Motor7 Running...\r\n");
            }

            if ((appCtrl.stateTimer > appCtrl.config.packMotor7TimeMs) &&
                (appCtrl.stateTimer <= (appCtrl.config.packMotor7TimeMs + 10))) {
                StopPackConveyor();
                StartWrapMotor(appCtrl.config.packWrapSpeed);
                #ifdef DEBUG_ENABLE
                printf("[APP] Packing Continue: Motor8 Running...\r\n");
                #endif
                Bluetooth_SendString("[APP] Packing Continue: Motor8 Running...\r\n");
            }

            if (appCtrl.stateTimer > (appCtrl.config.packMotor7TimeMs + appCtrl.config.packMotor8TimeMs)) {
                StopWrapMotor();
                appCtrl.stateTimer = 0;
                appCtrl.autoState = STEP_LOWER_RUN_SERVO;
            }
            break;

        case STEP_LOWER_RUN_SERVO:
			
            if(appCtrl.stateTimer <= 10) {
				Middle_Layer_Open();
				#ifdef DEBUG_ENABLE
                printf("[APP] Lower Belt SERVO...\r\n");
                #endif
				Bluetooth_SendString("[APP] Lower Belt SERVO...\r\n");
            }
            // 2. 运行指定时间到位
            if (appCtrl.stateTimer > appCtrl.config.servoTime) {
				Middle_Layer_Close();
                appCtrl.stateTimer = 0;
                appCtrl.autoState = STEP_RUNNING;  
            }
            break;
			
        // --- 稳定运行阶段 ---
        case STEP_RUNNING:
            appCtrl.runTime += 10; // 记录运行时间
            #ifdef DEBUG_ENABLE
            printf("[APP] STEP_RUNNING.\r\n");
            #endif
			appCtrl.stateTimer = 0;
			appCtrl.autoState = STEP_UPPER_RUN;
			HAL_Delay(600);
		
            break;
            
        // --- 停机阶段 (逆序：上->中->下) ---
        // 接收到 Stop 指令后会跳转到这里
        
        case STEP_STOP_UPPER:
            #ifdef DEBUG_ENABLE
            printf("[APP] Stopping Sequence: Stop Upper\r\n");
            #endif
            StopConveyorBelt(LAYER_UPPER); // 先停入口，不再进货
            appCtrl.stateTimer = 0;
            appCtrl.autoState = STEP_WAIT_EMPTY_1;
            break;
            
        case STEP_WAIT_EMPTY_1:
            // 等待中层的货物洗完
            if (appCtrl.stateTimer > appCtrl.config.stopDelayMs) {
                appCtrl.autoState = STEP_STOP_MIDDLE;
            }
            break;
            
        case STEP_STOP_MIDDLE:
            #ifdef DEBUG_ENABLE
            printf("[APP] Stopping Sequence: Stop Middle\r\n");
            #endif
            StopConveyorBelt(LAYER_MIDDLE); // 停中层
            appCtrl.stateTimer = 0;
            appCtrl.autoState = STEP_WAIT_EMPTY_2;
            break;
            
        case STEP_WAIT_EMPTY_2:
             // 等待下层把最后一点货送出去
            if (appCtrl.stateTimer > appCtrl.config.stopDelayMs) {
                appCtrl.autoState = STEP_STOP_LOWER;
            }
            break;
            
        case STEP_STOP_LOWER:
            #ifdef DEBUG_ENABLE
            printf("[APP] Stopping Sequence: Stop Lower.\r\n");
            #endif
            StopConveyorBelt(LAYER_LOWER); // 最后停出口
            appCtrl.autoState = STEP_DONE;
            break;
            
        case STEP_DONE:
            appCtrl.currentMode = MODE_IDLE; // 回到空闲模式
            break;
    }
}

/**
 * @brief 维护模式逻辑 (手动控制预留)
 */
static void Run_Maintenance_Mode(void) {
    // 维护模式通常由特定的调试指令控制，这里暂空
}

// --- 控制指令接口 ---
void App_Send_Cmd_StartAuto(void) 
{
    if (appCtrl.currentMode == MODE_IDLE) 
	{	
        appCtrl.currentMode = MODE_AUTO_CLEAN;
        appCtrl.autoState = STEP_READY; // 重置状态机
        appCtrl.isPaused = false;
    }
}

void App_Send_Cmd_Stop(void) 
{
    // 只有在自动运行模式下，才触发优雅停机序列
    if (appCtrl.currentMode == MODE_AUTO_CLEAN) 
	{
        // 如果正在运行，跳转到停机序列的第一步
        if (appCtrl.autoState == STEP_RUNNING || appCtrl.autoState < STEP_RUNNING) 
		{
            appCtrl.autoState = STEP_STOP_UPPER; 
        }
    } 
	else 
	{
        // 其他模式下直接强停
        StopWashingSystem();
        appCtrl.currentMode = MODE_IDLE;
    }
}

void App_Send_Cmd_Emergency(void) {
    EmergencyStopAll(); // Middle层提供的硬件急停
    appCtrl.currentMode = MODE_ERROR;
}

void App_Send_Cmd_Resume(void) {
    if (appCtrl.currentMode == MODE_ERROR) {
        ResetEmergencyStop(); // 解除硬件保护
        appCtrl.currentMode = MODE_IDLE;
        #ifdef DEBUG_ENABLE
        printf("[APP] Error Resumed.\r\n");
		Bluetooth_SendString("[APP] Error Resumed.\r\n");
        #endif
    }
}

SystemMode_t App_Get_CurrentMode(void) {
    return appCtrl.currentMode;
}

