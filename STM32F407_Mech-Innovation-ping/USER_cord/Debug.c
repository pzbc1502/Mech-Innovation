#include "debug.h"
#include "usart.h"

// 如果 usart.h 里没写 extern，这里必须补上，否则编译器不知道 huart6 是谁
extern UART_HandleTypeDef huart6;

// 重定向 printf 到 USART1
// 注意：必须在 Target 选项中勾选 "Use MicroLIB"
int fputc(int ch, FILE *f)
{
    // 这里的 1000 是超时时间(ms)。
    // 在高速控制循环中，如果串口断了或发太慢，会导致电机卡顿 1秒！
    // 建议平时开发用，正式比赛或高速运行时慎用 printf
    HAL_UART_Transmit(&huart6, (uint8_t *)&ch, 1, 1000);
    return ch;
}

// 重定向 scanf
int fgetc(FILE *f)
{
    uint8_t ch = 0;
    // 这里的 HAL_MAX_DELAY 会导致死等，直到收到字符
    HAL_UART_Receive(&huart6, &ch, 1, HAL_MAX_DELAY);
    return ch;
}

void Debug_Init(void)
{
    if (huart6.gState != HAL_UART_STATE_READY)
    {
        // 如果串口初始化，可以在这里初始化（不推荐，应在 main 初始化）
        // MX_USART1_UART_Init(); 
        printf("[ERROR] USART1 not initialized!\r\n");
        return;
    }
    printf("\r\n");
    printf("=============================================\r\n");
    printf("   STM32F407 Conveyor System Debugger\r\n");
    printf("   USART6 115200 bps Ready\r\n");
    printf("=============================================\r\n");
}

