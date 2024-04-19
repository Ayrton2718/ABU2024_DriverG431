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

        g_s2m.red_zone = HAL_GPIO_ReadPin(ZONE_SW_GPIO_Port, ZONE_SW_Pin);
        g_s2m.start_or_retry = HAL_GPIO_ReadPin(S_OR_R_SW_GPIO_Port, S_OR_R_SW_Pin);
        g_s2m.power_24v = !HAL_GPIO_ReadPin(POWER_SW_GPIO_Port, POWER_SW_Pin);
        g_s2m.start = !HAL_GPIO_ReadPin(START_SW_GPIO_Port, START_SW_Pin);
        g_s2m.boot = HAL_GPIO_ReadPin(BOOT_SW_GPIO_Port, BOOT_SW_Pin);
        g_s2m.kill = HAL_GPIO_ReadPin(KILL_SW_GPIO_Port, KILL_SW_Pin);

        uint8_t* buff = (uint8_t*)&g_s2m;
        buff[1] = buff[0]; 
        CSIo_sendUser(CSReg_0, (const uint8_t*)&g_s2m, sizeof(CSPanel_s2m_t));



        HAL_GPIO_WritePin(START_LED_GPIO_Port, START_LED_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BOOTED_LED_GPIO_Port, BOOTED_LED_Pin, (GPIO_PinState)g_m2s.booted);
        HAL_GPIO_WritePin(BOOTING_LED_GPIO_Port, BOOTING_LED_Pin, (GPIO_PinState)g_m2s.booting);
        HAL_GPIO_WritePin(BOOT_ERR_LED_GPIO_Port, BOOT_ERR_LED_Pin, (GPIO_PinState)g_m2s.boot_err);
        HAL_GPIO_WritePin(RED_ZONE_LED_GPIO_Port, RED_ZONE_LED_Pin, (GPIO_PinState)g_m2s.is_red_zone);
        HAL_GPIO_WritePin(BLUE_ZONE_LED_GPIO_Port, BLUE_ZONE_LED_Pin, (GPIO_PinState)g_m2s.is_blue_zone);
        
		if (g_m2s.io_err) {
			HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
			__HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 100);

		}else if (g_m2s.ros_err1) {
			HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
			__HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 100);

		}else if (g_m2s.ros_err2) {
			HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
			__HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 100);
		}else {
			HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_4);
			__HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_4, 0);
		}
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
	if((reg == CSReg_0) && (len == sizeof(CSPanel_m2s_t)) && (data[0] == data[1]))
	{
        g_m2s = *(CSPanel_m2s_t*)data;
		return CSTYPE_TRUE;
    }
	return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}

