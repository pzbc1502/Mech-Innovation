#include "app_key_task.h"
#include "oled.h"
#include "OLED_Data.h"
#include "main.h"
#include "app_Conveyor.h"
#include "Emm_v5.h"
#include "bsp_relay.h"


extern uint8_t flag_JiDianQi;

//ű

void btn_one_cb(flex_button_t *btn)
{
    switch (btn->event)
    {
        case FLEX_BTN_PRESS_LONG_START: // ?

            break;

        case FLEX_BTN_PRESS_CLICK:		// ź
			printf("Start Auto\r\n");
			flag_JiDianQi = 1;
//			Set_Relay_Switch(RELAY_PUMP, 1);
//			Set_Relay_Switch(RELAY_CUTTER, 1);
			OLED_Clear();
			OLED_ShowString(0, 16, "CMD: START   ", OLED_8X16);
			OLED_ShowString(0, 32, "code:", OLED_8X16);
			OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
			OLED_Update();
		
			//??
			App_Send_Cmd_StartAuto();
			
            
		
            break;
        default:
            break;
    }
}

void btn_two_cb(flex_button_t *btn)
{
    switch (btn->event)
    {
        case FLEX_BTN_PRESS_CLICK: // ?
			
			flag_JiDianQi = 1;
//			Set_Relay_Switch(RELAY_PUMP, 1);
//			Set_Relay_Switch(RELAY_CUTTER, 1);
			OLED_Clear();
			OLED_ShowString(0, 16, "CMD:REAST    ", OLED_8X16);
			OLED_ShowString(0, 32, "code:", OLED_8X16);
			OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
			OLED_Update();			
			
			//?
			App_Send_Cmd_Resume();
			
		
			printf("Resume/Reset\r\n");
            
            break;

        case FLEX_BTN_PRESS_LONG_START:

            break;
        default:
            break;
    }
}

void btn_three_cb(flex_button_t *btn)
{
    switch (btn->event)
    {
        case FLEX_BTN_PRESS_CLICK: // ??
			
			flag_JiDianQi = 0;
			OLED_Clear();
			Set_Relay_Switch(RELAY_PUMP, 0);
			Set_Relay_Switch(RELAY_CUTTER, 0);
			OLED_ShowString(0, 16, "CMD: STOP_SEQ", OLED_8X16);
			OLED_ShowString(0, 32, "code:", OLED_8X16);
			OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
			OLED_Update();	  
			
			//??
            App_Send_Cmd_Stop();
            
            printf("Graceful Stop\r\n");
			
            break;

        case FLEX_BTN_PRESS_LONG_START:
            break;
        default:
            break;
    }
}   

void btn_four_cb(flex_button_t *btn)
{
    switch (btn->event)
    {
        case FLEX_BTN_PRESS_CLICK://ź
			
			flag_JiDianQi = 0;
			Set_Relay_Switch(RELAY_PUMP, 0);
			Set_Relay_Switch(RELAY_CUTTER, 0);
			OLED_Clear();
			OLED_ShowString(0, 16, "CMD: STOP!!!", OLED_8X16);
			OLED_ShowString(0, 32, "code:", OLED_8X16);
			OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
			OLED_Update();
			  
			//???
			App_Send_Cmd_Emergency();		
			
			
			printf("EMERGENCY STOP\r\n");
		
            break;		
			
        case FLEX_BTN_PRESS_LONG_HOLD://ź
						break;
        case FLEX_BTN_PRESS_LONG_HOLD_UP://??ź
						break;
        default:break;
    }
}
