#ifndef __TOUCH_WATER_H
#define __TOUCH_WATER_H

#include "main.h"

#define TOUCH_WATER_READ_TIMES  10  // 多次采样平均次数

uint16_t TOUCH_WATER_ADC_Read(void);
uint16_t TOUCH_WATER_GetData(void);

#endif


