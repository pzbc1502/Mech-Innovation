#include "HX711.h"
#include "delay_us.h"


HX711_Data_t hx711;

/******************************************************************
 * 函数说明：HX711初始化（复位SCK）
 ******************************************************************/
void HX711_Init(void)
{
    HX711_SCK_LOW();
    HAL_Delay(10);
    Get_Maopi(); // 上电自动去皮
}

/******************************************************************
 * 函数说明：读取HX711 AD值
 ******************************************************************/
uint32_t HX711_Read(void)
{
    uint32_t count = 0;
    uint8_t i;

    // 等待DT线拉低，表示转换完成
    // 为了防止死循环，实际工程中最好加超时判断
    if(HX711_DT_READ() == 1) return 0; 

    delay_us(1); // 这里的延时是为了满足时序要求

    // 读取24位数据
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

    // 第25个脉冲，设置增益为128
    HX711_SCK_HIGH();
    delay_us(1);
    count = count ^ 0x800000; // 转换符号位
    HX711_SCK_LOW();
    delay_us(1);

    return count;
}

/******************************************************************
 * 函数说明：获取毛皮（去皮）
 ******************************************************************/
void Get_Maopi(void)
{
    hx711.Weight_Maopi = HX711_Read();
}



/******************************************************************
 * 函数说明：获取实际重量
 ******************************************************************/
float Get_Weight(void)
{
    hx711.Buffer = HX711_Read();
    
    // 简单的防抖动逻辑，可根据需要优化
    if(hx711.Buffer == 0) return hx711.Weight_Real;

    if(hx711.Buffer > hx711.Weight_Maopi)
    {
        hx711.Weight_Shiwu = hx711.Buffer - hx711.Weight_Maopi;
        hx711.Weight_Real = (float)hx711.Weight_Shiwu / GapValue;
    }
    else
    {
        // 如果测量值小于毛皮值，说明出现负重或误差
        hx711.Weight_Real = 0;
    }
    
    return hx711.Weight_Real;
}

