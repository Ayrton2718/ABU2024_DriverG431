#include "user_task.h"
#include "can_smbus/can_smbus.hpp"
#include "main.h"
#include "cs_panel_type.h"

#include <array>
#define BUZZER_HANDLE ((&htim3))
#define BUZZER_CHANNEL ((TIM_CHANNEL_4))

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static bool g_disable_24v = false;
static uint32_t g_disable_24v_count = 0;

static CSPanel_s2m_t g_s2m;
static CSPanel_m2s_t g_m2s;

static bool g_rst_flg;
static CSTimer_t g_tim;
//----------------------
void UserTask_setup(void)
{
    g_rst_flg = false;

    g_disable_24v = false;
    g_disable_24v_count = 0;

    HAL_TIM_PWM_Start(BUZZER_HANDLE, BUZZER_CHANNEL);
    __HAL_TIM_SET_COMPARE(BUZZER_HANDLE, BUZZER_CHANNEL, 0);

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

        g_s2m.red_zone = !HAL_GPIO_ReadPin(SW_ZONE_GPIO_Port, SW_ZONE_Pin);
        g_s2m.start_or_retry = HAL_GPIO_ReadPin(SW_RETRY_GPIO_Port, SW_RETRY_Pin);
        if(g_disable_24v){
        	g_s2m.power_24v = 1;
        }else{
            g_s2m.power_24v = !HAL_GPIO_ReadPin(SW_24V_GPIO_Port, SW_24V_Pin);
        }
        g_s2m.start = !HAL_GPIO_ReadPin(SW_START_GPIO_Port, SW_START_Pin);
        g_s2m.boot = HAL_GPIO_ReadPin(SW_BOOT_GPIO_Port, SW_BOOT_Pin);
        g_s2m.kill = HAL_GPIO_ReadPin(SW_KILL_GPIO_Port, SW_KILL_Pin);
        g_s2m.strategy = HAL_GPIO_ReadPin(SW_STRATEGY_GPIO_Port, SW_STRATEGY_Pin);

        if(g_s2m.boot && g_s2m.kill){
			g_disable_24v_count++;
			if(100 < g_disable_24v_count){
				g_disable_24v = true;
			}

			g_s2m.boot = 0;
			g_s2m.kill = 1;
		}else{
			g_disable_24v_count = 0;
		}

        uint8_t* buff = (uint8_t*)&g_s2m;
        buff[1] = buff[0]; 
        CSIo_sendUser(CSReg_0, (const uint8_t*)&g_s2m, sizeof(CSPanel_s2m_t));

        HAL_GPIO_WritePin(LED_RUNNING_GPIO_Port, LED_RUNNING_Pin, (GPIO_PinState)g_m2s.running);
        HAL_GPIO_WritePin(LED_BOOTING_GPIO_Port, LED_BOOTING_Pin, (GPIO_PinState)(g_m2s.booting | g_disable_24v));
        HAL_GPIO_WritePin(LED_BOOT_ERR_GPIO_Port, LED_BOOT_ERR_Pin, (GPIO_PinState)g_m2s.boot_err);
        HAL_GPIO_WritePin(LED_RED_ZONE_GPIO_Port, LED_RED_ZONE_Pin, (GPIO_PinState)g_m2s.is_red_zone);
        HAL_GPIO_WritePin(LED_BLUE_ZONE_GPIO_Port, LED_BLUE_ZONE_Pin, (GPIO_PinState)g_m2s.is_blue_zone);
        HAL_GPIO_WritePin(LED_RETRY_GPIO_Port, LED_RETRY_Pin, (GPIO_PinState)g_m2s.retry);
        HAL_GPIO_WritePin(LED_STRATEGY_GPIO_Port, LED_STRATEGY_Pin, (GPIO_PinState)g_m2s.strategy);
        
		if (g_m2s.io_err) {
			__HAL_TIM_SET_COMPARE(BUZZER_HANDLE, BUZZER_CHANNEL, 100);

		}else {
			__HAL_TIM_SET_COMPARE(BUZZER_HANDLE, BUZZER_CHANNEL, 0);
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

