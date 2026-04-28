#include "HX711.h"
#include "delay_us.h"

HX711_Data_t hx711;

void HX711_Init(void)
{
    HX711_SCK_LOW();
    HAL_Delay(10);
    HX711_Tare(HX711_TARE_SAMPLES);
}

uint32_t HX711_Read(void)
{
    uint32_t count = 0;
    uint32_t tickStart = HAL_GetTick();
    uint8_t i;

    while(HX711_DT_READ() == 1)
    {
        if((HAL_GetTick() - tickStart) >= HX711_READY_TIMEOUT_MS)
        {
            return 0;
        }
    }

    delay_us(1);

    for(i = 0; i < 24; i++)
    {
        HX711_SCK_HIGH();
        delay_us(1);
        count = count << 1;
        HX711_SCK_LOW();
        delay_us(1);
        if(HX711_DT_READ())
        {
            count++;
        }
    }

    HX711_SCK_HIGH();
    delay_us(1);
    count = count ^ 0x800000;
    HX711_SCK_LOW();
    delay_us(1);

    return count;
}

void HX711_Tare(uint8_t samples)
{
    uint64_t sum = 0;
    uint8_t validCount = 0;

    if(samples == 0)
    {
        samples = 1;
    }

    for(uint8_t i = 0; i < samples; i++)
    {
        uint32_t value = HX711_Read();
        if(value != 0)
        {
            sum += value;
            validCount++;
        }
        HAL_Delay(HX711_SAMPLE_DELAY_MS);
    }

    if(validCount > 0)
    {
        hx711.Weight_Maopi = (uint32_t)(sum / validCount);
    }
}

void Get_Maopi(void)
{
    HX711_Tare(HX711_TARE_SAMPLES);
}

float Get_Weight(void)
{
    hx711.Buffer = HX711_Read();

    if(hx711.Buffer == 0)
    {
        return hx711.Weight_Real;
    }

    if(hx711.Buffer > hx711.Weight_Maopi)
    {
        hx711.Weight_Shiwu = (int32_t)(hx711.Buffer - hx711.Weight_Maopi);
        hx711.Weight_Real = (float)hx711.Weight_Shiwu / GapValue;
    }
    else
    {
        hx711.Weight_Shiwu = 0;
        hx711.Weight_Real = 0.0f;
    }

    return hx711.Weight_Real;
}

float HX711_GetStableWeight(uint8_t samples)
{
    float sum = 0.0f;
    uint8_t validCount = 0;

    if(samples == 0)
    {
        samples = 1;
    }

    for(uint8_t i = 0; i < samples; i++)
    {
        uint32_t raw = HX711_Read();
        if(raw != 0)
        {
            hx711.Buffer = raw;

            if(raw > hx711.Weight_Maopi)
            {
                hx711.Weight_Shiwu = (int32_t)(raw - hx711.Weight_Maopi);
                hx711.Weight_Real = (float)hx711.Weight_Shiwu / GapValue;
            }
            else
            {
                hx711.Weight_Shiwu = 0;
                hx711.Weight_Real = 0.0f;
            }

            sum += hx711.Weight_Real;
            validCount++;
        }

        HAL_Delay(HX711_SAMPLE_DELAY_MS);
    }

    if(validCount > 0)
    {
        hx711.Weight_Real = sum / validCount;
    }

    return hx711.Weight_Real;
}
