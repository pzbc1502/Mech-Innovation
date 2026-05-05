#include "touch_water.h"
#include "adc.h"

/*
 * Water level sensor input:
 *   PA1 -> ADC1_IN1
 * CubeMX configures hadc1 as single-channel ADC_CHANNEL_1.
 */
uint16_t TOUCH_WATER_ADC_Read(void)
{
    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        return (uint16_t)HAL_ADC_GetValue(&hadc1);
    }

    return 0;
}

uint16_t TOUCH_WATER_GetData(void)
{
    uint32_t tempData = 0;
    uint8_t i;

    for (i = 0; i < TOUCH_WATER_READ_TIMES; i++)
    {
        tempData += TOUCH_WATER_ADC_Read();
        HAL_Delay(5);
    }

    tempData /= TOUCH_WATER_READ_TIMES;
    return (uint16_t)tempData;
}
