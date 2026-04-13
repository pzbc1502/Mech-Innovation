#include "bsp_bluetooth.h"
#include "K230_cmd.h"

uint8_t BT_RxBuffer;
char BT_RxPacket[BT_RX_MAX_LEN] = {0};
volatile uint8_t BT_RxFlag = 0;   // 建议加 volatile

volatile static uint8_t RxIndex = 0;
volatile static uint8_t RxState = 0; // 0:等待包头, 1:接收数据

void Bluetooth_Init(void)
{
    HAL_UART_Receive_IT(&huart2, &BT_RxBuffer, 1);
}

void Bluetooth_SendByte(uint8_t Byte)
{
    HAL_UART_Transmit(&huart2, &Byte, 1, 1000);
}

void Bluetooth_SendString(char *String)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)String, strlen(String), 1000);
}

void Bluetooth_Printf(char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Bluetooth_SendString(String);
}

/******************************************************************
 * 串口接收回调：解析 @xxx<回车/换行> 格式
 * 兼容：
 *   @011\r\n
 *   @011\n
 *   @011\r
 ******************************************************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        uint8_t ch = BT_RxBuffer;

        if(RxState == 0)
        {
            // 只要看到 '@' 就认为是新包头，不再受 BT_RxFlag 限制
            if(ch == '@')
            {
                RxState = 1;
                RxIndex = 0;
                memset(BT_RxPacket, 0, BT_RX_MAX_LEN);
            }
        }
        else if(RxState == 1)
        {
            // 遇到 '\r' 或 '\n' 都认为一帧结束（兼容性更强）
            if(ch == '\r' || ch == '\n')
            {
                BT_RxPacket[RxIndex] = '\0';   // 封尾
                BT_RxFlag = 1;                 // 标记接收完成
                RxState = 0;                   // 回到等待包头
            }
            else
            {
                if(RxIndex < BT_RX_MAX_LEN - 1)
                {
                    BT_RxPacket[RxIndex++] = ch;
                }
                else
                {
                    // 溢出，丢弃本帧
                    RxState = 0;
                    RxIndex = 0;
                }
            }
        }
        // 继续开下一次中断接收
        HAL_UART_Receive_IT(&huart2, &BT_RxBuffer, 1);
    }
	else if(huart->Instance == UART4){
		// 调用 k230_cmd.c 里的函数
        K230_UART4_RxHandler(huart);
	}
}

