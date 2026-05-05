#include "app_bluetooth_task.h" 
#include "bsp_bluetooth.h"
#include "app_Conveyor.h"
#include "OLED.h"
// ...
extern uint8_t flag_JiDianQi;

//处理蓝牙指令
void Process_Bluetooth_Command(void)
{
    if (BT_RxFlag == 1) // 收到完整数据包
    {
        Bluetooth_Printf("[BT] Recv: %s\r\n", BT_RxPacket); // 调试打印

        // 简单的字符串匹配
        // 使用 strncmp 防止后面有 \r\n 导致匹配失败
        if (strncmp(BT_RxPacket, "011", 3) == 0)
        {
            Bluetooth_Printf("[BT] CMD: START\r\n");

			//启动状态机
			App_Send_Cmd_StartAuto();
			
			flag_JiDianQi = 1;
//			Set_Relay_Switch(RELAY_PUMP, 1);
//			Set_Relay_Switch(RELAY_CUTTER, 1);
			OLED_Clear();
			OLED_ShowString(0, 16, "CMD: START   ", OLED_8X16);
			OLED_ShowString(0, 32, "code:", OLED_8X16);
			OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
			OLED_Update();
			
            printf("Start Auto\r\n");
			
            // 这里也可以调用 OLED 显示函数更新 UI
        }
        else if (strncmp(BT_RxPacket, "022", 3) == 0)
        {
            Bluetooth_Printf("[BT] CMD: RESET\r\n");

			//复位
			App_Send_Cmd_Resume();
			
			flag_JiDianQi = 1;
//			Set_Relay_Switch(RELAY_PUMP, 1);
//			Set_Relay_Switch(RELAY_CUTTER, 1);
			OLED_Clear();
			OLED_ShowString(0, 16, "CMD:REAST    ", OLED_8X16);
			OLED_ShowString(0, 32, "code:", OLED_8X16);
			OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
			OLED_Update();			
						
        }
        else if (strncmp(BT_RxPacket, "033", 3) == 0)
        {
            Bluetooth_Printf("[BT] CMD: STOP SEQ\r\n");
			//优雅停止
            App_Send_Cmd_Stop();
			
			
			flag_JiDianQi = 0;
			OLED_Clear();
			Set_Relay_Switch(RELAY_PUMP, 0);
			Set_Relay_Switch(RELAY_CUTTER, 0);
			OLED_ShowString(0, 16, "CMD: STOP_SEQ", OLED_8X16);
			OLED_ShowString(0, 32, "code:", OLED_8X16);
			OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
			OLED_Update();	  
			
			
        }
        else if (strncmp(BT_RxPacket, "044", 3) == 0)
        {
            Bluetooth_Printf("[BT] CMD: STOP!!!\r\n");
			
			//强制停止
			App_Send_Cmd_Emergency();				
			
			
			flag_JiDianQi = 0;
			Set_Relay_Switch(RELAY_PUMP, 0);
			Set_Relay_Switch(RELAY_CUTTER, 0);
			OLED_Clear();
			OLED_ShowString(0, 16, "CMD: ESTOP!!!", OLED_8X16);
			OLED_ShowString(0, 32, "code:", OLED_8X16);
			OLED_ShowChar(48, 32, '0' + flag_JiDianQi, OLED_8X16);
			OLED_Update();
			
			
        }
        else
        {
            Bluetooth_Printf("[BT] Unknown CMD\r\n");
        }

        // 清除标志位，准备下一次接收
        BT_RxFlag = 0; 
        // 记得清空 Buffer，防止残留数据影响下一次解析
        memset(BT_RxPacket, 0, BT_RX_MAX_LEN);
    }
	
}


