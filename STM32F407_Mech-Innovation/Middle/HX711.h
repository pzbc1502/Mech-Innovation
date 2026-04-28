#ifndef __HX711_H_
#define __HX711_H_

#include "main.h"

#define HX711_SCK_LOW()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET)
#define HX711_SCK_HIGH()    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET)
#define HX711_DT_READ()     HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11)

#define GapValue 207.0f
#define HX711_READY_TIMEOUT_MS 100U
#define HX711_TARE_SAMPLES 10U
#define HX711_WEIGHT_SAMPLES 10U
#define HX711_SAMPLE_DELAY_MS 20U

typedef struct {
    uint32_t Buffer;
    uint32_t Weight_Maopi;
    int32_t Weight_Shiwu;
    float Weight_Real;
} HX711_Data_t;

extern HX711_Data_t hx711;

void HX711_Init(void);
uint32_t HX711_Read(void);
void HX711_Tare(uint8_t samples);
void Get_Maopi(void);
float Get_Weight(void);
float HX711_GetStableWeight(uint8_t samples);

#endif
