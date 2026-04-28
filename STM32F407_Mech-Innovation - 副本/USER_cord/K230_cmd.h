#ifndef __K230_CMD_H__
#define __K230_CMD_H__

#include "main.h"      // 替换原来的 stm32f4xx.h
#include <string.h>
#include <stdio.h>

// 帧结构常量
#define FRAME_HEAD1 0x2C
#define FRAME_HEAD2 0x12
#define FRAME_TAIL  0x5B
#define CATEGORY_LEN 6 // 类别固定6字节长度

// 类别枚举
typedef enum {
    CATEGORY_UNKNOWN,
    CATEGORY_YELLOW,
    CATEGORY_ROOT,
} Category_Type;

// 接收状态枚举
typedef enum {
    STATE_WAIT_HEAD1,        // 等待帧头1
    STATE_WAIT_HEAD2,        // 等待帧头2
    STATE_RECV_CATEGORY,     // 接收类别（6字节）
    STATE_RECV_X_HIGH,       // 接收X高字节
    STATE_RECV_X_LOW,        // 接收X低字节
    STATE_RECV_Y_HIGH,       // 接收Y高字节
    STATE_RECV_Y_LOW,        // 接收Y低字节
    STATE_WAIT_TAIL          // 等待帧尾
} UART_RecvState;

// 接收数据结构
typedef struct {
    uint8_t category[CATEGORY_LEN]; // 类别（6字节）
    Category_Type category_type;    // 类别枚举
    uint16_t x;                     // 0~800（双字节拼接）
    uint16_t y;                     // 0~480（双字节拼接）
    uint8_t is_valid;               // 数据有效标志 1-有效 0-无效
} K230_Data;

// 外部全局变量声明
extern K230_Data k230_recv_data;

// 函数声明
void bsp_uart4_start_receive(void); // 
void stm32_send_frame(void);        // STM32发送帧函数
void uart4_send_byte(uint8_t byte); // 单字节发送函数
void K230_UART4_RxHandler(UART_HandleTypeDef *huart);

#endif

