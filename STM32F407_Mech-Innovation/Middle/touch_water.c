#include "touch_water.h"
#include "adc.h"   // CubeMX 生成的 ADC 初始化头文件

/**
  * @brief  读取一次 ADC 原始值
  * @retval ADC 转换值 (0~4095)
  */
uint16_t TOUCH_WATER_ADC_Read(void)
{
    /* 启动 ADC 转换 */
    HAL_ADC_Start(&hadc1);

    /* 等待转换完成，超时时间 100ms */
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        /* 返回 ADC 转换值 */
        return (uint16_t)HAL_ADC_GetValue(&hadc1);
    }

    return 0;
}

/**
  * @brief  获取经过多次采样平均后的水位 ADC 数据
  * @retval 平均后的 ADC 值 (0~4095)
  */
uint16_t TOUCH_WATER_GetData(void)
{
    uint32_t tempData = 0;

    for (uint8_t i = 0; i < TOUCH_WATER_READ_TIMES; i++)
    {
        tempData += TOUCH_WATER_ADC_Read();
        HAL_Delay(5);  // HAL库延时，替代原来的 delay_ms
    }

    tempData /= TOUCH_WATER_READ_TIMES;
    return (uint16_t)tempData;
}