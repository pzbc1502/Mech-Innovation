#include "HX711.h"
#include "delay_us.h"

HX711_Data_t hx711_left = {
    HX711_LEFT_SCK_PORT,
    HX711_LEFT_SCK_PIN,
    HX711_LEFT_DT_PORT,
    HX711_LEFT_DT_PIN,
    0,
    0,
    0,
    0.0f,
    false
};

HX711_Data_t hx711_right = {
    HX711_RIGHT_SCK_PORT,
    HX711_RIGHT_SCK_PIN,
    HX711_RIGHT_DT_PORT,
    HX711_RIGHT_DT_PIN,
    0,
    0,
    0,
    0.0f,
    false
};

HX711_Data_t hx711;

/* 将公开通道枚举映射到对应的状态和引脚表。 */
static HX711_Data_t *HX711_GetDevice(HX711_Channel_t channel)
{
    switch (channel)
    {
        case HX711_CHANNEL_LEFT:
            return &hx711_left;
        case HX711_CHANNEL_RIGHT:
            return &hx711_right;
        default:
            return NULL;
    }
}

static void HX711_WriteSck(HX711_Data_t *dev, GPIO_PinState state)
{
    HAL_GPIO_WritePin(dev->sckPort, dev->sckPin, state);
}

static GPIO_PinState HX711_ReadDt(HX711_Data_t *dev)
{
    return HAL_GPIO_ReadPin(dev->dtPort, dev->dtPin);
}

static void HX711_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 两路 HX711 都在这里配置，避免 CubeMX 重新生成时丢失 PB14/PB15。 */
    HAL_GPIO_WritePin(GPIOB, HX711_LEFT_SCK_PIN | HX711_RIGHT_SCK_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = HX711_LEFT_SCK_PIN | HX711_RIGHT_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = HX711_LEFT_DT_PIN | HX711_RIGHT_DT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void HX711_Init(void)
{
    HX711_GPIO_Init();

    /* SCK 保持低电平，让 HX711 处于上电并等待转换的状态。 */
    HX711_WriteSck(&hx711_left, GPIO_PIN_RESET);
    HX711_WriteSck(&hx711_right, GPIO_PIN_RESET);
    HAL_Delay(10);

    /* 上电时左右两侧分别去皮，应用层最终重量为左侧 + 右侧。 */
    HX711_TareChannel(HX711_CHANNEL_LEFT, HX711_TARE_SAMPLES);
    HX711_TareChannel(HX711_CHANNEL_RIGHT, HX711_TARE_SAMPLES);
    hx711 = hx711_left;
}

uint32_t HX711_ReadChannel(HX711_Channel_t channel)
{
    HX711_Data_t *dev = HX711_GetDevice(channel);
    uint32_t count = 0;
    uint32_t tickStart = HAL_GetTick();
    uint8_t i;

    if (dev == NULL)
    {
        return 0;
    }

    /* HX711 未就绪时 DT 为高电平；超时可以避免模块缺失时永久阻塞。 */
    while (HX711_ReadDt(dev) == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - tickStart) >= HX711_READY_TIMEOUT_MS)
        {
            dev->isValid = false;
            return 0;
        }
    }

    delay_us(1);

    /* 读取 24 位补码 ADC 数据，高位先出。 */
    for (i = 0; i < 24; i++)
    {
        HX711_WriteSck(dev, GPIO_PIN_SET);
        delay_us(1);
        count = count << 1;
        HX711_WriteSck(dev, GPIO_PIN_RESET);
        delay_us(1);

        if (HX711_ReadDt(dev) == GPIO_PIN_SET)
        {
            count++;
        }
    }

    /* 第 25 个脉冲用于选择下一次转换的增益/通道。 */
    HX711_WriteSck(dev, GPIO_PIN_SET);
    delay_us(1);
    count = count ^ 0x800000;
    HX711_WriteSck(dev, GPIO_PIN_RESET);
    delay_us(1);

    dev->isValid = true;
    return count;
}

uint32_t HX711_Read(void)
{
    return HX711_ReadChannel(HX711_CHANNEL_LEFT);
}

void HX711_TareChannel(HX711_Channel_t channel, uint8_t samples)
{
    HX711_Data_t *dev = HX711_GetDevice(channel);
    uint64_t sum = 0;
    uint8_t validCount = 0;
    uint8_t i;

    if (dev == NULL)
    {
        return;
    }

    if (samples == 0)
    {
        samples = 1;
    }

    /* 对多次有效原始采样求平均，作为该称重通道的零点偏移。 */
    for (i = 0; i < samples; i++)
    {
        uint32_t value = HX711_ReadChannel(channel);
        if (value != 0)
        {
            sum += value;
            validCount++;
        }
        HAL_Delay(HX711_SAMPLE_DELAY_MS);
    }

    if (validCount > 0)
    {
        dev->Weight_Maopi = (uint32_t)(sum / validCount);
    }
}

void HX711_Tare(uint8_t samples)
{
    HX711_TareChannel(HX711_CHANNEL_LEFT, samples);
    hx711 = hx711_left;
}

void Get_Maopi(void)
{
    HX711_Tare(HX711_TARE_SAMPLES);
}

float HX711_GetWeightChannel(HX711_Channel_t channel)
{
    HX711_Data_t *dev = HX711_GetDevice(channel);
    uint32_t raw;

    if (dev == NULL)
    {
        return 0.0f;
    }

    raw = HX711_ReadChannel(channel);
    dev->Buffer = raw;

    if (raw == 0)
    {
        dev->Weight_Shiwu = 0;
        dev->Weight_Real = 0.0f;
        dev->isValid = false;
        return 0.0f;
    }

    /* 负载小于零时钳位为 0，因为称重板只需要输出正向净重量。 */
    if (raw > dev->Weight_Maopi)
    {
        dev->Weight_Shiwu = (int32_t)(raw - dev->Weight_Maopi);
        dev->Weight_Real = (float)dev->Weight_Shiwu / GapValue;
    }
    else
    {
        dev->Weight_Shiwu = 0;
        dev->Weight_Real = 0.0f;
    }

    dev->isValid = true;
    return dev->Weight_Real;
}

float Get_Weight(void)
{
    float weight = HX711_GetWeightChannel(HX711_CHANNEL_LEFT);
    hx711 = hx711_left;
    return weight;
}

float HX711_GetStableWeight(uint8_t samples)
{
    float sum = 0.0f;
    uint8_t validCount = 0;
    uint8_t i;

    if (samples == 0)
    {
        samples = 1;
    }

    for (i = 0; i < samples; i++)
    {
        float weight = HX711_GetWeightChannel(HX711_CHANNEL_LEFT);
        if (HX711_IsChannelValid(HX711_CHANNEL_LEFT))
        {
            sum += weight;
            validCount++;
        }
        HAL_Delay(HX711_SAMPLE_DELAY_MS);
    }

    if (validCount > 0)
    {
        hx711_left.Weight_Real = sum / (float)validCount;
    }

    hx711 = hx711_left;
    return hx711.Weight_Real;
}

float HX711_GetCombinedWeight(float *leftWeight, float *rightWeight)
{
    float left = HX711_GetWeightChannel(HX711_CHANNEL_LEFT);
    float right = HX711_GetWeightChannel(HX711_CHANNEL_RIGHT);

    /* 缺失或超时的一侧按 0 处理，避免沿用旧数据。 */
    if (!HX711_IsChannelValid(HX711_CHANNEL_LEFT))
    {
        left = 0.0f;
    }

    if (!HX711_IsChannelValid(HX711_CHANNEL_RIGHT))
    {
        right = 0.0f;
    }

    if (leftWeight != NULL)
    {
        *leftWeight = left;
    }

    if (rightWeight != NULL)
    {
        *rightWeight = right;
    }

    hx711.Weight_Real = left + right;
    hx711.Weight_Shiwu = (int32_t)(hx711.Weight_Real * GapValue);
    hx711.isValid = HX711_IsChannelValid(HX711_CHANNEL_LEFT) || HX711_IsChannelValid(HX711_CHANNEL_RIGHT);

    return hx711.Weight_Real;
}

bool HX711_IsChannelValid(HX711_Channel_t channel)
{
    HX711_Data_t *dev = HX711_GetDevice(channel);

    if (dev == NULL)
    {
        return false;
    }

    return dev->isValid;
}

bool HX711_IsChannelReady(HX711_Channel_t channel)
{
    HX711_Data_t *dev = HX711_GetDevice(channel);

    if (dev == NULL)
    {
        return false;
    }

    /* DT 为低表示可以立即读取；应用层用它保持非阻塞调度。 */
    return HX711_ReadDt(dev) == GPIO_PIN_RESET;
}
