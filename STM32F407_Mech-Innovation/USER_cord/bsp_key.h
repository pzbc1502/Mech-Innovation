#ifndef _BSP_KEY_H_
#define _BSP_KEY_H_


#define GPIO_KEY_PORT	GPIOD
#define GPIO_KEY_PIN_ONE_PIN	GPIO_PIN_4
#define GPIO_KEY_PIN_TWO_PIN	GPIO_PIN_5
#define GPIO_KEY_PIN_THREE_PIN	GPIO_PIN_7
#define GPIO_KEY_PIN_FOUR_PIN	GPIO_PIN_6


typedef struct {
    unsigned int one : 1; // 使用位字段来节省空间
    unsigned int two : 1;
    unsigned int three : 1;
	unsigned int four : 1;
} KEY_STATUS;

void Key_Init(void);
KEY_STATUS key_scan(void);

#endif
