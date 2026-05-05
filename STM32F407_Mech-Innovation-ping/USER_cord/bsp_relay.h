/*
 * bsp_relay.h
 * 
 * 继电器驱动头文件 (STM32 HAL库版本) - 支持多路控制
 */
#ifndef _BSP_RELAY_H_
#define _BSP_RELAY_H_

#include "main.h" 

// --- 硬件引脚定义 ---
// 假设两个继电器都在 GPIOA，如果不是，请分别定义 Port
#ifndef RELAY_GPIO_PORT
#define RELAY_GPIO_PORT GPIOA
#endif

// 定义具体的引脚号
#define RELAY_PIN_1     GPIO_PIN_5  // 继电器1 (例如：切刀)
#define RELAY_PIN_2     GPIO_PIN_6  // 继电器2 (例如：水泵)

// --- ID 定义 (给 App 层调用时用) ---
typedef enum {
    RELAY_ID_1 = 1,  // 切刀
    RELAY_ID_2 = 2   // 水泵
} RelayID_t;

// 为了代码可读性，可以加个别名
#define RELAY_CUTTER    RELAY_ID_1
#define RELAY_PUMP      RELAY_ID_2

// --- 控制逻辑状态 ---
// 低电平触发：0=吸合/工作，1=释放/停止
// 如果是高电平触发，请交换下面两个宏的值
#define RELAY_STATE_OFF   GPIO_PIN_RESET // 吸合
#define RELAY_STATE_ON  GPIO_PIN_SET   // 释放

// --- 函数声明 ---
void Relay_Init(void);

/**
 * @brief 控制继电器
 * @param id:    RELAY_ID_1 或 RELAY_ID_2
 * @param state: 1 = 开启(吸合), 0 = 关闭(释放)
 */
void Set_Relay_Switch(uint8_t id, uint8_t state);

#endif /* _BSP_RELAY_H_ */

