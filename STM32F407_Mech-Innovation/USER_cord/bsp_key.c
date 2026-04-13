#include "bsp_key.h"
#include "gpio.h"                  // Device header

void Key_Init(void)
{
	MX_GPIO_Init();
}

KEY_STATUS key_scan(void)
{
    KEY_STATUS states;
    // 读取每个按键的状态
    states.one = HAL_GPIO_ReadPin(GPIO_KEY_PORT, GPIO_KEY_PIN_ONE_PIN) ? 1 : 0;
    states.two = HAL_GPIO_ReadPin(GPIO_KEY_PORT, GPIO_KEY_PIN_TWO_PIN) ? 1 : 0;
    states.three = HAL_GPIO_ReadPin(GPIO_KEY_PORT, GPIO_KEY_PIN_THREE_PIN) ? 1 : 0;
    states.four = HAL_GPIO_ReadPin(GPIO_KEY_PORT, GPIO_KEY_PIN_FOUR_PIN) ? 1 : 0;

    return states;

}
