#ifndef __HX711_H_
#define __HX711_H_

#include "main.h" // 包含HAL库定义和GPIO定义

// 宏定义操作引脚，使用CubeMX中定义的Label或直接操作
#define HX711_SCK_LOW()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET)
#define HX711_SCK_HIGH()    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET)
#define HX711_DT_READ()     HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11)

// 校准参数，需要根据实际传感器调整
#define GapValue 207.0f 

// 结构体管理
typedef struct {
    uint32_t Buffer;
    uint32_t Weight_Maopi; // 毛皮（去皮值）
    int32_t Weight_Shiwu;  // 实物值
    float Weight_Real;     // 实际重量(g)
} HX711_Data_t;

extern HX711_Data_t hx711;

// 函数声明
void HX711_Init(void);
uint32_t HX711_Read(void);
void Get_Maopi(void);
float Get_Weight(void);

#endif


