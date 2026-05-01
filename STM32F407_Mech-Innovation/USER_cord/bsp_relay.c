/*
 * bsp_relay.c
 */
#include "bsp_relay.h"

/**
  * @brief  继电器初始化
  * @note   初始化所有继电器引脚为关闭状态
  */
void Relay_Init(void)
{
    // 确保初始状态为释放 (高电平)
    HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_PIN_1, RELAY_STATE_OFF);
    HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_PIN_2, RELAY_STATE_OFF);
}

/**
  * @brief  设置继电器开关状态
  * @param  id:    RELAY_ID_1 (切刀) 或 RELAY_ID_2 (水泵)
  * @param  state: 1 - 吸合 (ON/工作), 0 - 释放 (OFF/停止)
  */
void Set_Relay_Switch(uint8_t id, uint8_t state)
{
    // 根据传入的 state (1或0) 决定物理电平
    GPIO_PinState pinState = (state == 1) ? RELAY_STATE_ON : RELAY_STATE_OFF;

    switch(id)
    {
        case RELAY_ID_1:
            HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_PIN_1, pinState);
            break;
            
        case RELAY_ID_2:
            HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_PIN_2, pinState);
            break;
            
        default:
            // 防止传入错误的ID，什么都不做
            break;
    }
}

