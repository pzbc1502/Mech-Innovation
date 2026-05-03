#ifndef _BSP_BLUETOOTH_H_
#define _BSP_BLUETOOTH_H_

#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Bluetooth module uses UART4: TX=PA0, RX=PC11. */
extern UART_HandleTypeDef huart4;

#define BT_RX_MAX_LEN 150

extern uint8_t BT_RxBuffer;
extern char BT_RxPacket[BT_RX_MAX_LEN];
extern volatile uint8_t BT_RxFlag;

void Bluetooth_Init(void);
void Bluetooth_SendByte(uint8_t Byte);
void Bluetooth_SendString(char *String);
void Bluetooth_Printf(char *format, ...);

#endif
