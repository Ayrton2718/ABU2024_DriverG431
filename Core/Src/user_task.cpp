#include "user_task.h"
#include "can_smbus/can_smbus.hpp"
#include "main.h"
#include "cs_panel_type.h"

#include <array>
#define PWM_HANDLE ((&htim3))

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

CSPanel_s2m_t g_s2m;
CSPanel_m2s_t g_m2s;

static bool g_rst_flg;
static CSTimer_t g_tim;
//----------------------
void UserTask_setup(void)
{
    g_rst_flg = false;

//    g_sw_pins[0] = std::make_pair(Zone_SW_GPIO_Port, Zone_SW_Pin);			//A|B
//    g_sw_pins[1] = std::make_pair(R_Start_SW_GPIO_Port, R_Start_SW_Pin);	//Start|Reset
//    g_sw_pins[2] = std::make_pair(Power_GPIO_Port, Power_Pin);				//24V
//    g_sw_pins[3] = std::make_pair(Start_SW_GPIO_Port, Start_SW_Pin);		//Start Switch
//    g_sw_pins[4] = std::make_pair(Boot_SW_GPIO_Port, Boot_SW_Pin);			//Boot Switch
//    g_sw_pins[5] = std::make_pair(Kill_SW_GPIO_Port, Kill_SW_Pin);			//Kill Switch

//    HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
//    __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 500);

    // g_error1.type =0;
    // g_error2.type =0;

    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_PANEL, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_tim);
    if(10000 < us)
    {
        CSTimer_start(&g_tim);

//        for(size_t i = 0; i < g_sw_pins.size(); i++)
//        {
//            if(HAL_GPIO_ReadPin(g_sw_pins[i].first, g_sw_pins[i].second) == GPIO_PIN_SET){
//                set_sw(&g_sw_reg.list, i, false);
//            }else{
//                set_sw(&g_sw_reg.list, i, true);
//            }
//        }
//        Zone	 = HAL_GPIO_ReadPin(Zone_SW_GPIO_Port, Zone_SW_Pin);
//        if(Zone){
//			HAL_GPIO_WritePin(GPIOB, ZoneBlue_LED_Pin, GPIO_PIN_SET);
//			HAL_GPIO_WritePin(GPIOA, ZoneRed_LED_Pin, GPIO_PIN_RESET);
//        }
//        else {
//			HAL_GPIO_WritePin(GPIOB, ZoneBlue_LED_Pin, GPIO_PIN_RESET);
//			HAL_GPIO_WritePin(GPIOA, ZoneRed_LED_Pin, GPIO_PIN_SET);
//        }

/*スイッチ，LEDテスト用のプログラム-------------------------------------------------------------
      	 //Boot	 = HAL_GPIO_ReadPin(Boot_SW_GPIO_Port, Boot_SW_Pin);
      	 //Kill	 = HAL_GPIO_ReadPin(Kill_SW_GPIO_Port, Kill_SW_Pin);

      	 Start	 = HAL_GPIO_ReadPin(Power_GPIO_Port, Power_Pin);
      	 noise	 = HAL_GPIO_ReadPin(R_Start_SW_GPIO_Port, R_Start_SW_Pin);
      	 	 	   HAL_GPIO_ReadPin(Start_SW_GPIO_Port, Start_SW_Pin);


      	 if(Boot)	HAL_GPIO_WritePin(GPIOA, BootNow_LED_Pin, GPIO_PIN_SET);
      	 else 		HAL_GPIO_WritePin(GPIOA, BootNow_LED_Pin, GPIO_PIN_RESET);

      	 if(Kill)	HAL_GPIO_WritePin(GPIOA, BootErr_LED_Pin, GPIO_PIN_SET);
      	 else 		HAL_GPIO_WritePin(GPIOA, BootErr_LED_Pin, GPIO_PIN_RESET);

      	 if(Start)HAL_GPIO_WritePin(GPIOA, Start_LED_Pin, GPIO_PIN_SET);
      	 else HAL_GPIO_WritePin(GPIOA, Start_LED_Pin, GPIO_PIN_RESET);

      	 if(noise)	{
      		 HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
      		 __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 0);
      	 }
      	 else{
      		 HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
      		 __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 100);
      	 }

      	 if(Zone){
      		 	 	HAL_GPIO_WritePin(GPIOB, ZoneBlue_LED_Pin, GPIO_PIN_SET);
      		 	 	HAL_GPIO_WritePin(GPIOA, ZoneRed_LED_Pin, GPIO_PIN_RESET);
      	 }
      	 else {
      		 	 	HAL_GPIO_WritePin(GPIOB, ZoneBlue_LED_Pin, GPIO_PIN_RESET);
      		 	 	HAL_GPIO_WritePin(GPIOA, ZoneRed_LED_Pin, GPIO_PIN_SET);
      	 }
*///--------------------------------------------------------------------------------------

		//  CSIo_sendUser(CSReg_0, (const uint8_t*)&g_sw_reg, sizeof(switch_t));
    }

    if(g_rst_flg)
    {
        g_rst_flg = false;
    }
}

void UserTask_unsafeLoop(void)
{
    UserTask_loop();
}

static void UserTask_timerCallback(void)
{
}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len)
{
	// if((reg == CSReg_0) && (len == sizeof(panel_t)))
	{
		// g_error1= *((const panel_t*)data);
		// if (g_error1.type & (1<<0)) {
		// 	 HAL_GPIO_WritePin(GPIOA, BootOK_LED_Pin , GPIO_PIN_SET);
		// 	 HAL_GPIO_WritePin(GPIOA, BootNow_LED_Pin, GPIO_PIN_RESET);
		// 	 HAL_GPIO_WritePin(GPIOA, BootErr_LED_Pin, GPIO_PIN_RESET);

		// }else if (g_error1.type & (1<<1)) {
		// 	 HAL_GPIO_WritePin(GPIOA, BootOK_LED_Pin , GPIO_PIN_RESET);
		// 	 HAL_GPIO_WritePin(GPIOA, BootNow_LED_Pin, GPIO_PIN_SET);
		// 	 HAL_GPIO_WritePin(GPIOA, BootErr_LED_Pin, GPIO_PIN_RESET);

		// }else if (g_error1.type & (1<<2)) {
		// 	 HAL_GPIO_WritePin(GPIOA, BootOK_LED_Pin , GPIO_PIN_RESET);
		// 	 HAL_GPIO_WritePin(GPIOA, BootNow_LED_Pin, GPIO_PIN_RESET);
		// 	 HAL_GPIO_WritePin(GPIOA, BootErr_LED_Pin, GPIO_PIN_SET);
		// }
		return CSTYPE_TRUE;
	// }else if((reg == CSReg_1) && (len == sizeof(panel_t)))
	// {
		// g_error2= *((const panel_t*)data);
		// if (g_error2.type & (1<<0)) {
		// 	HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
		// 	__HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 100);

		// }else if (g_error2.type & (1<<1)) {
		// 	HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
		// 	__HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 100);

		// }else if (g_error2.type & (1<<2)) {
		// 	HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
		// 	__HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 100);
		// }else {
		// 	HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
		// 	__HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 0);
		// }
		/*まだ詳細が決まっていないROS Error用の予約-----------------------------
		 if (g_error2.type & (1<<4)) {
			 HAL_GPIO_WritePin(GPIOA, BootOK_LED_Pin , GPIO_PIN_SET);
			 HAL_GPIO_WritePin(GPIOA, BootNow_LED_Pin, GPIO_PIN_RESET);
			 HAL_GPIO_WritePin(GPIOA, BootErr_LED_Pin, GPIO_PIN_RESET);

		}else if (g_error2.type & (1<<5)) {
			 HAL_GPIO_WritePin(GPIOA, BootOK_LED_Pin , GPIO_PIN_RESET);
			 HAL_GPIO_WritePin(GPIOA, BootNow_LED_Pin, GPIO_PIN_SET);
			 HAL_GPIO_WritePin(GPIOA, BootErr_LED_Pin, GPIO_PIN_RESET);

		}else if (g_error2.type & (1<<6)) {
			 HAL_GPIO_WritePin(GPIOA, BootOK_LED_Pin , GPIO_PIN_RESET);
			 HAL_GPIO_WritePin(GPIOA, BootNow_LED_Pin, GPIO_PIN_RESET);
			 HAL_GPIO_WritePin(GPIOA, BootErr_LED_Pin, GPIO_PIN_SET);
		}
		 ------------------------------------------------------------------*/
		return CSTYPE_TRUE;
	}
	return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}

