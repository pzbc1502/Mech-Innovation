#ifndef _APP_KEY_TASK_H_
#define _APP_KEY_TASK_H_

#include "mid_button.h"

void set_app_key_current_mode(char mode);
char get_app_key_current_mode(void);

void btn_one_cb(flex_button_t *btn);
void btn_two_cb(flex_button_t *btn);
void btn_three_cb(flex_button_t *btn);
void btn_four_cb(flex_button_t *btn);

#endif
