/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "OLED.h"
#include "DEBUG.h"
#include "bsp_key.h"
#include "bsp_relay.h"
#include "bsp_bluetooth.h"
#include "SERVO.h"
#include "Emm_V5.h"
#include "Conveyor_belt.h"
#include "mid_button.h"

#include "Servo_Ctrl.h"
#include "HX711.h"

#include "app_bluetooth_task.h"
#include "app_Conveyor.h"
#include "app_key_task.h"
#include "K230_cmd.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// 继电器初始化标志位（0：释放，1：吸合）
uint8_t flag_JiDianQi = 0;  
	
// 定义时间戳变量
static uint32_t last_bluetooth_time = 0;
static uint32_t last_scan_time = 0;
static uint32_t last_wish_time= 0;
static uint32_t last_ui_time = 0;

// K230 最新视觉结果缓存（主循环消费）
static uint16_t k230_last_x = 0;
static uint16_t k230_last_y = 0;
static Category_Type k230_last_type = CATEGORY_UNKNOWN;
static uint32_t k230_last_rx_time = 0;


// ========== USART6 (电机驱动) 接收缓冲区 ==========
MOTOR_RX_TypeDef motor_rx_data_t[MOTOR_BUFFER_QUANTITY];
volatile uint8_t motor_buff_ctrl = 0;       						// DMA 写入索引
volatile uint8_t motor_process_ctrl = 0;    						// 主循环处理索引
volatile bool motor_new_data_flag = false;  						// 新数据标志        	// 实际接收长度

//电机中断服务
void USART6_DMAHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE) && 
        __HAL_UART_GET_IT_SOURCE(&huart6, UART_IT_IDLE))
    {
		//清除 IDLE 标志
        __HAL_UART_CLEAR_IDLEFLAG(&huart6);
		//停止 DMA，防止干扰
        HAL_UART_DMAStop(&huart6);

        // 计算接收长度
        motor_rx_data_t[motor_buff_ctrl].size = UART6_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart6.hdmarx);

        // 切换到下一个 buffer
        motor_buff_ctrl = (motor_buff_ctrl + 1) % MOTOR_BUFFER_QUANTITY;

        // 重启 DMA 到新 buffer
        HAL_UART_Receive_DMA(&huart6, motor_rx_data_t[motor_buff_ctrl].buffer, UART6_BUFFER_SIZE);

        // 通知主循环（如果需要进一步处理）
        motor_new_data_flag = true;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_UART4_Init();
  MX_CAN2_Init();
  /* USER CODE BEGIN 2 */
   
	 
	USER_CAN2_Filter_Init();																	// 初始化CAN滤波器
	if(HAL_CAN_Start(&hcan2) != HAL_OK) { Error_Handler(); }	// 启动CAN控制器
	if(HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) { Error_Handler(); }	// 使能CAN控制器接收中断
	
  	//USART6,电机驱动
	__HAL_UART_CLEAR_IDLEFLAG(&huart6); 																	// 清除可能残留的 IDLE 标志
	__HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE); 															// 使能 USART2 的 IDLE 中断
	HAL_UART_Receive_DMA(&huart6, motor_rx_data_t[motor_buff_ctrl].buffer, UART6_BUFFER_SIZE); 				// 启动 DMA 循环接收    

	//用户初始化
	OLED_Init();
	OLED_Clear();
	Key_Init();
	user_button_init(); 
	Debug_Init();
	Bluetooth_Init();
	bsp_uart4_start_receive();
	HX711_Init();
	Relay_Init();
	ServoSystem_Init();
	


	//任务初始化
	App_Conwashing_Init();
	printf("System Init Success!\r\n");
	
	//PA5是切割			PA6是水泵
	// 低电平触发：0=释放（高电平），1=吸合（低电平）
//	Set_Relay_Switch(RELAY_PUMP, 0);
//	Set_Relay_Switch(RELAY_CUTTER, 0);  
    // 初始OLED显示
    OLED_ShowString(0, 32, "code:", OLED_8X16);
    OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
	OLED_Update();	
	
	Bluetooth_Printf("System Init Success! Waiting for commands...\r\n");
	printf("System Init Success! Waiting for commands...\r\n");
	 
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	
    uint32_t now = HAL_GetTick();
	
	// --- 任务1：应用层调度 (每10ms执行一次)
    if (now - last_wish_time >= 10)
    {
        last_wish_time = now;

//        // 消费 K230 新数据（由 UART4 中断生产）
//        __disable_irq();
//        if (k230_recv_data.is_valid)
//        {
//            k230_last_x = k230_recv_data.x;
//            k230_last_y = k230_recv_data.y;
//            k230_last_type = k230_recv_data.category_type;
//            k230_recv_data.is_valid = 0;
//            k230_last_rx_time = now;
//        }
//        __enable_irq();


        App_Conwashing_Task();  // 运行传送带状态机
    }

	
    // --- 任务2：按键扫描 (每30ms执行一次)
    if (now - last_scan_time >= 30)
    {
        last_scan_time = now;
        
        flex_button_scan();     // 扫描按键
    }
	
    // --- 任务3：蓝牙扫描 (每50ms执行一次)
    if (now - last_bluetooth_time >= 50)
    {
        last_bluetooth_time = now;
        
        Process_Bluetooth_Command();     // 扫描蓝牙		
    }	
	
    // --- 任务4：UI 刷新 (每500ms执行一次) 
    if (now - last_ui_time >= 500)
    {
				last_ui_time = now;
						
				// 显示当前模式
				OLED_ShowString(0, 0, "Mode:", OLED_8X16);
				SystemMode_t mode = App_Get_CurrentMode();

				switch(mode) {
				case MODE_IDLE:      OLED_ShowString(48, 0, "Idle ", OLED_8X16); break;
				case MODE_AUTO_CLEAN:OLED_ShowString(48, 0, "Auto ", OLED_8X16); break;
				case MODE_ERROR:     OLED_ShowString(48, 0, "Error", OLED_8X16); break;
				default:             OLED_ShowString(48, 0, "Wait ", OLED_8X16); break;
				}

//        OLED_ShowString(0, 16, "K230:", OLED_8X16);
//        switch(k230_last_type) {
//            case CATEGORY_YELLOW: OLED_ShowString(48, 16, "YEL ", OLED_8X16); break;
//            case CATEGORY_ROOT:   OLED_ShowString(48, 16, "ROOT", OLED_8X16); break;
//            default:              OLED_ShowString(48, 16, "UNK ", OLED_8X16); break;
//        }

//        OLED_ShowString(0, 32, "X:", OLED_8X16);
//        OLED_ShowNum(16, 32, k230_last_x, 3, OLED_8X16);
//        OLED_ShowString(56, 32, "Y:", OLED_8X16);
//        OLED_ShowNum(72, 32, k230_last_y, 3, OLED_8X16);

//        OLED_ShowString(0, 48, "V:", OLED_8X16);
//        OLED_ShowNum(16, 48, (uint32_t)(now - k230_last_rx_time), 4, OLED_8X16);

        OLED_Update();	  
	}

	
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	while(1);

  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
