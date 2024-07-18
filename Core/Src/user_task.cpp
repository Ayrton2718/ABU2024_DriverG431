#include "user_task.h"
#include "can_smbus/can_smbus.hpp"
#include "tim.h"

#include <array>

#define LED_N (7)

extern "C" {

typedef struct{
    uint8_t r;
    uint8_t g;
    uint8_t b;
}__attribute__((__packed__)) rgb_t;

}


static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);
static void UserTask_runLED(const std::array<rgb_t, LED_N>* red_arr);

static uint16_t g_dma_buff[(24*LED_N)+50];
static volatile bool g_send_complete = true;

static rgb_t g_rgb_reg;

static bool g_rst_flg;
static CSTimer_t g_out_tim;
static CSTimer_t g_cmd_tim;

void UserTask_setup(void)
{
    g_rgb_reg.r = 0;
    g_rgb_reg.g = 0;
    g_rgb_reg.b = 0;

    g_rst_flg = false;

    CSTimer_start(&g_out_tim);
    CSTimer_start(&g_cmd_tim);
    CSIo_bind(CSType_appid_UNKNOWN, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t ms = CSTimer_getMs(g_out_tim);
    if(10 < ms)
    {
        CSTimer_start(&g_out_tim);

		std::array<rgb_t, LED_N> red_arr;
		std::fill(red_arr.begin(), red_arr.end(), g_rgb_reg);
		UserTask_runLED(&red_arr);
    }

    if(g_rst_flg)
    {
        g_rst_flg = false;
    }
}

void UserTask_unsafeLoop(void)
{
	uint32_t ms = CSTimer_getMs(g_out_tim);
	if(10 < ms)
	{
		CSTimer_start(&g_out_tim);

		uint32_t cmd_ms = CSTimer_getMs(g_cmd_tim);
		if(500 < cmd_ms){
			CSTimer_start(&g_cmd_tim);
			cmd_ms = 0;
		}

		uint8_t bright;
		if(cmd_ms < 250){
			bright = 125 - static_cast<uint8_t>(cmd_ms * (125.0f / 250.0f));
		}else if(cmd_ms < 500){
			cmd_ms -= 250;
			bright = static_cast<uint8_t>(cmd_ms * (125.0f / 250.0f));
		}

		rgb_t rgb = {0, bright, 0};
		std::array<rgb_t, LED_N> red_arr;
		std::fill(red_arr.begin(), red_arr.end(), rgb);
		UserTask_runLED(&red_arr);
	}
}

static void UserTask_timerCallback(void)
{
}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len)
{
    if((reg == CSReg_0) && (len == sizeof(rgb_t)))
    {
    	g_rgb_reg = *((const rgb_t*)data);
        return CSTYPE_TRUE;
    }
    return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}

static void UserTask_runLED(const std::array<rgb_t, LED_N>* red_arr){
	if(!g_send_complete){
		CSLed_err();
		return;
	}

	uint32_t pwm_index=0;
	for (auto it = red_arr->begin(); it != red_arr->end(); it++)
	{
		uint32_t color;
		color = ((it->g<<16) | (it->r<<8) | (it->b));

		for (int i=23; i>=0; i--)
		{
			if (color&(1<<i))
			{
				g_dma_buff[pwm_index] = 6;  // 2/3 of 90
			}

			else g_dma_buff[pwm_index] = 3;  // 1/3 of 90

			pwm_index++;
		}

	}

	for (int i=0; i<50; i++)
	{
		g_dma_buff[pwm_index] = 0;
		pwm_index++;
	}

	g_send_complete = false;
	HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t *)g_dma_buff, pwm_index);
}


void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
	g_send_complete=true;
}
