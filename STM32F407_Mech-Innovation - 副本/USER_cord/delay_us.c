#include "delay_us.h"
#include "main.h"

void delay_us(uint32_t us)
{
	us *= 42; // 校准因子，需实测调整
	while (us--)
	{
	__NOP(); __NOP(); __NOP(); __NOP(); // 4个NOP，防止被优化
	}
}
