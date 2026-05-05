#ifndef __HX711_H_
#define __HX711_H_

#include "main.h"
#include <stdbool.h>

/* 第三代称重板使用两套独立 HX711。
 * GPIO 在 HX711_Init() 内部初始化，因此 CubeMX .ioc 暂不需要配置 PB14/PB15。
 */
#define HX711_LEFT_SCK_PORT     GPIOB
#define HX711_LEFT_SCK_PIN      GPIO_PIN_10
#define HX711_LEFT_DT_PORT      GPIOB
#define HX711_LEFT_DT_PIN       GPIO_PIN_11

#define HX711_RIGHT_SCK_PORT    GPIOB
#define HX711_RIGHT_SCK_PIN     GPIO_PIN_14
#define HX711_RIGHT_DT_PORT     GPIOB
#define HX711_RIGHT_DT_PIN      GPIO_PIN_15

/* 兼容旧版单 HX711 调用路径。
 * 这些宏固定指向左侧通道。
 */
#define HX711_SCK_LOW()         HAL_GPIO_WritePin(HX711_LEFT_SCK_PORT, HX711_LEFT_SCK_PIN, GPIO_PIN_RESET)
#define HX711_SCK_HIGH()        HAL_GPIO_WritePin(HX711_LEFT_SCK_PORT, HX711_LEFT_SCK_PIN, GPIO_PIN_SET)
#define HX711_DT_READ()         HAL_GPIO_ReadPin(HX711_LEFT_DT_PORT, HX711_LEFT_DT_PIN)

/* GapValue 是 HX711 原始计数换算为克的校准系数。 */
#define GapValue 207.0f
#define HX711_READY_TIMEOUT_MS 100U
#define HX711_TARE_SAMPLES 10U
#define HX711_WEIGHT_SAMPLES 10U
#define HX711_SAMPLE_DELAY_MS 20U

typedef enum {
    HX711_CHANNEL_LEFT = 0,
    HX711_CHANNEL_RIGHT = 1,
    HX711_CHANNEL_COUNT = 2
} HX711_Channel_t;

typedef struct {
    /* 每个通道自带引脚配置，左右 HX711 可以复用同一套读取/去皮逻辑。 */
    GPIO_TypeDef *sckPort;
    uint16_t sckPin;
    GPIO_TypeDef *dtPort;
    uint16_t dtPin;
    uint32_t Buffer;
    uint32_t Weight_Maopi;
    int32_t Weight_Shiwu;
    float Weight_Real;
    bool isValid;
} HX711_Data_t;

/* hx711 保留为左侧通道的兼容镜像，避免旧调用方断编译。 */
extern HX711_Data_t hx711;
extern HX711_Data_t hx711_left;
extern HX711_Data_t hx711_right;

void HX711_Init(void);
uint32_t HX711_Read(void);
uint32_t HX711_ReadChannel(HX711_Channel_t channel);
void HX711_Tare(uint8_t samples);
void HX711_TareChannel(HX711_Channel_t channel, uint8_t samples);
void Get_Maopi(void);
float Get_Weight(void);
float HX711_GetStableWeight(uint8_t samples);
float HX711_GetWeightChannel(HX711_Channel_t channel);

/* 返回左侧 + 右侧合成重量；无效通道按 0 参与计算。 */
float HX711_GetCombinedWeight(float *leftWeight, float *rightWeight);
bool HX711_IsChannelValid(HX711_Channel_t channel);

/* 非阻塞就绪检查，供 10ms 应用调度调用。 */
bool HX711_IsChannelReady(HX711_Channel_t channel);

#endif
