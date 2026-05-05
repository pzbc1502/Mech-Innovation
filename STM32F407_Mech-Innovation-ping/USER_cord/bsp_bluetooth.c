#include "bsp_bluetooth.h"

uint8_t BT_RxBuffer;
char BT_RxPacket[BT_RX_MAX_LEN] = {0};
volatile uint8_t BT_RxFlag = 0;

static volatile uint8_t RxIndex = 0;
static volatile uint8_t RxState = 0; /* 0: wait '@', 1: receive payload */

void Bluetooth_Init(void)
{
    HAL_UART_Receive_IT(&huart4, &BT_RxBuffer, 1);
}

void Bluetooth_SendByte(uint8_t Byte)
{
    HAL_UART_Transmit(&huart4, &Byte, 1, 1000);
}

void Bluetooth_SendString(char *String)
{
    HAL_UART_Transmit(&huart4, (uint8_t *)String, strlen(String), 1000);
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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4)
    {
        uint8_t ch = BT_RxBuffer;

        if (RxState == 0)
        {
            if (ch == '@')
            {
                RxState = 1;
                RxIndex = 0;
                memset(BT_RxPacket, 0, BT_RX_MAX_LEN);
            }
        }
        else
        {
            if ((ch == '\r') || (ch == '\n'))
            {
                BT_RxPacket[RxIndex] = '\0';
                BT_RxFlag = 1;
                RxState = 0;
            }
            else if (RxIndex < (BT_RX_MAX_LEN - 1))
            {
                BT_RxPacket[RxIndex++] = (char)ch;
            }
            else
            {
                RxState = 0;
                RxIndex = 0;
            }
        }

        HAL_UART_Receive_IT(&huart4, &BT_RxBuffer, 1);
    }
}
