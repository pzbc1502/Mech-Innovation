#ifndef _BSP_BLUETOOTH_H_
#define _BSP_BLUETOOTH_H_

#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// 定义使用的串口句柄，CubeMX生成默认为 huart2
extern UART_HandleTypeDef huart2;

// 接收相关定义
#define BT_RX_MAX_LEN 150
extern uint8_t BT_RxBuffer;          // 单字节接收缓存
extern char BT_RxPacket[BT_RX_MAX_LEN]; // 完整数据包
extern volatile uint8_t BT_RxFlag;            // 接收完成标志

// 函数声明
void Bluetooth_Init(void);
void Bluetooth_SendByte(uint8_t Byte);
void Bluetooth_SendString(char *String);
void Bluetooth_Printf(char *format, ...);
void Bluetooth_HandleRx(void); // 在主循环中调用处理数据

#endif

