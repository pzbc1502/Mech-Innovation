#include "K230_cmd.h"
#include "usart.h"     

// 全局变量
K230_Data k230_recv_data = {0};

static UART_RecvState recv_state = STATE_WAIT_HEAD1;
static uint8_t category_buf[CATEGORY_LEN] = {0}; // 临时存储类别6字节
static uint8_t category_idx = 0;                 // 类别接收索引
static uint8_t x_high = 0, x_low = 0;            // X坐标高低字节
static uint8_t y_high = 0, y_low = 0;            // Y坐标高低字节
static uint8_t uart4_rx_byte = 0;                // HAL库需要指定的接收缓存(单字节)

static void k230_parser_reset(void)
{
    recv_state = STATE_WAIT_HEAD1;
    category_idx = 0;
    x_high = 0;
    x_low = 0;
    y_high = 0;
    y_low = 0;
    memset(category_buf, 0, sizeof(category_buf));
}

static uint8_t to_lower_ascii(uint8_t c)
{
    if(c >= 'A' && c <= 'Z')
    {
        return (uint8_t)(c + ('a' - 'A'));
    }
    return c;
}


/*==================================================================
 * 串口发送部分
 *==================================================================*/

// 单字节发送函数
void uart4_send_byte(uint8_t byte)
{
    // HAL库单字节发送，阻塞等待（超时设为HAL_MAX_DELAY）
    HAL_UART_Transmit(&huart4, &byte, 1, HAL_MAX_DELAY);
}

// STM32发送start帧函数
void stm32_send_frame(void)
{
    // 帧头
    uart4_send_byte(FRAME_HEAD1);
    uart4_send_byte(FRAME_HEAD2);
    // 标志位 "start"
    uint8_t start_str[] = "start";
    for(uint8_t i=0; i<5; i++)
    {
        uart4_send_byte(start_str[i]);
    }
    // 帧尾
    uart4_send_byte(FRAME_TAIL);
}

// 类别字符串转枚举（兼容大小写，ROOT 允许后2字节为任意填充）
static Category_Type category_str_to_type(const uint8_t *category)
{
    if((to_lower_ascii(category[0]) == 'y') &&
       (to_lower_ascii(category[1]) == 'e') &&
       (to_lower_ascii(category[2]) == 'l') &&
       (to_lower_ascii(category[3]) == 'l') &&
       (to_lower_ascii(category[4]) == 'o') &&
       (to_lower_ascii(category[5]) == 'w'))
    {
        return CATEGORY_YELLOW;
    }

    if((to_lower_ascii(category[0]) == 'r') &&
       (to_lower_ascii(category[1]) == 'o') &&
       (to_lower_ascii(category[2]) == 'o') &&
       (to_lower_ascii(category[3]) == 't'))
    {
        return CATEGORY_ROOT;
    }

    return CATEGORY_UNKNOWN;
}


/*==================================================================
 * 串口接收与状态机解析部分 (HAL库回调方式)
 *==================================================================*/

// 启动一次中断接收（需在main函数中调用一次）
void bsp_uart4_start_receive(void)
{
    k230_parser_reset();
    HAL_UART_Receive_IT(&huart4, &uart4_rx_byte, 1);
}

// 串口接收完成回调函数
void K230_UART4_RxHandler(UART_HandleTypeDef *huart)
{
    if((huart == NULL) || (huart->Instance != UART4))
    {
        return;
    }

    uint8_t rx_byte = uart4_rx_byte; // 读取接收到的单字节

    // 状态机解析帧
    switch(recv_state)
    {
        case STATE_WAIT_HEAD1:
            if(rx_byte == FRAME_HEAD1)
            {
                recv_state = STATE_WAIT_HEAD2; // 匹配帧头1，等待帧头2
            }
            break;

        case STATE_WAIT_HEAD2:
            if(rx_byte == FRAME_HEAD2)
            {
                recv_state = STATE_RECV_CATEGORY; // 匹配帧头2，接收类别
                category_idx = 0;
                memset(category_buf, 0, sizeof(category_buf));
            }
            else if(rx_byte == FRAME_HEAD1)
            {
                recv_state = STATE_WAIT_HEAD2; // 处理连续帧头，提高抗干扰能力
            }
            else
            {
                recv_state = STATE_WAIT_HEAD1; // 帧头2匹配失败，重置
            }
            break;

        case STATE_RECV_CATEGORY:
            // 接收6字节类别
            category_buf[category_idx++] = rx_byte;
            if(category_idx >= CATEGORY_LEN) // 类别接收完成
            {
                recv_state = STATE_RECV_X_HIGH; // 开始接收X高字节
            }
            break;

        case STATE_RECV_X_HIGH:
            x_high = rx_byte; // 存储X高字节
            recv_state = STATE_RECV_X_LOW;
            break;

        case STATE_RECV_X_LOW:
            x_low = rx_byte; // 存储X低字节
            recv_state = STATE_RECV_Y_HIGH;
            break;

        case STATE_RECV_Y_HIGH:
            y_high = rx_byte; // 存储Y高字节
            recv_state = STATE_RECV_Y_LOW;
            break;

        case STATE_RECV_Y_LOW:
            y_low = rx_byte; // 存储Y低字节
            recv_state = STATE_WAIT_TAIL;
            break;

        case STATE_WAIT_TAIL:
            if(rx_byte == FRAME_TAIL)
            {
                // 帧尾匹配成功，拼接双字节坐标
                k230_recv_data.x = (uint16_t)(((uint16_t)x_high << 8) | x_low);
                k230_recv_data.y = (uint16_t)(((uint16_t)y_high << 8) | y_low);

                // 限制坐标范围：X(0~800) Y(0~480)
                k230_recv_data.x = (k230_recv_data.x > 800U) ? 800U : k230_recv_data.x;
                k230_recv_data.y = (k230_recv_data.y > 480U) ? 480U : k230_recv_data.y;

                // 拷贝类别、转换类型
                memcpy(k230_recv_data.category, category_buf, CATEGORY_LEN);
                k230_recv_data.category_type = category_str_to_type(category_buf);
                k230_recv_data.is_valid = 1; // 数据有效
            }
            else
            {
                // 帧尾匹配失败，清空有效标志，避免上层误用旧数据
                k230_recv_data.is_valid = 0;
                k230_recv_data.category_type = CATEGORY_UNKNOWN;
            }
            k230_parser_reset();
            break;

        default:
            k230_parser_reset();
            break;
    }

    // 解析完毕后，重新开启接收中断（由于每次只接收1字节，必须重复开启）
    HAL_UART_Receive_IT(&huart4, &uart4_rx_byte, 1);
}




