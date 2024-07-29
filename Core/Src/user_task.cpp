#include "user_task.h"
#include "can_smbus/can_smbus.hpp"
#include "tim.h"

#include <array>

#define R1
//#define R2

#ifdef R1
#define LED1_N (6)
#define LED2_N (6)
#define LED_BRIGHT_RATE (0.5)
#elif R2
#define LED1_N (7)
#define LED2_N (0)
#define LED_BRIGHT_RATE (1.0)
#endif /*R2*/

extern "C" {

typedef struct{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t hz; // hz * 10
}__attribute__((__packed__)) led_tape_t;

}

struct rgb_t{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};


static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);
static void UserTask_runLED(const std::array<rgb_t, LED1_N + LED2_N>* red_arr);

static uint16_t g_dma_buff[(24*(LED1_N + LED2_N))+50];
static volatile bool g_send_complete = true;

static CSTimer_t g_cmd_tim[2];
static led_tape_t g_rgb_reg[2];

static bool g_rst_flg;
static CSTimer_t g_out_tim;

void UserTask_setup(void)
{
    g_rgb_reg[0].r = 255;
    g_rgb_reg[0].g = 255;
    g_rgb_reg[0].b = 255;
    g_rgb_reg[0].hz = 2 * 10;

    g_rgb_reg[1].r = 255;
    g_rgb_reg[1].g = 255;
    g_rgb_reg[1].b = 255;
    g_rgb_reg[1].hz = 2 * 10;
    CSTimer_start(&g_cmd_tim[0]);
    CSTimer_start(&g_cmd_tim[1]);

    g_rst_flg = false;

    CSTimer_start(&g_out_tim);
    CSIo_bind(CSType_appid_UNKNOWN, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t ms = CSTimer_getMs(g_out_tim);
	if(10 < ms)
	{
		CSTimer_start(&g_out_tim);

        rgb_t rgb[2];
        for(size_t i = 0; i < 2; i++)
        {
            float bright_rate;
            if(g_rgb_reg[i].hz != 0){
                uint32_t current_ms = CSTimer_getMs(g_cmd_tim[i]);
                uint32_t order_ms = 10000 / static_cast<uint32_t>(g_rgb_reg[i].hz);
                if(order_ms < current_ms){
                    CSTimer_start(&g_cmd_tim[i]);
                    current_ms = 0;
                }

                if(current_ms < (order_ms/2)){
                    bright_rate = current_ms / static_cast<float>(order_ms/2);
                }else if(current_ms){
                    current_ms -= (order_ms/2);
                    bright_rate = 1 - current_ms / static_cast<float>(order_ms/2);
                }
            }else{
                bright_rate = 1;
            }

            bright_rate *= LED_BRIGHT_RATE;
            rgb[i].r = static_cast<uint8_t>(g_rgb_reg[i].r * bright_rate);
            rgb[i].g = static_cast<uint8_t>(g_rgb_reg[i].g * bright_rate);
            rgb[i].b = static_cast<uint8_t>(g_rgb_reg[i].b * bright_rate);
        }

		std::array<rgb_t, LED1_N + LED2_N> red_arr;
        for(size_t i = 0; i < LED1_N+LED2_N; i++)
        {
            if(i < LED1_N){
                red_arr[i] = rgb[0];
            }else{
                red_arr[i] = rgb[1];
            }
        }

		UserTask_runLED(&red_arr);
	}

    if(g_rst_flg)
    {
        g_rst_flg = false;
    }
}

void UserTask_unsafeLoop(void)
{
    g_rgb_reg[0].r = 255;
    g_rgb_reg[0].g = 255;
    g_rgb_reg[0].b = 255;
    g_rgb_reg[0].hz = 2 * 10;

    g_rgb_reg[1].r = 255;
    g_rgb_reg[1].g = 255;
    g_rgb_reg[1].b = 255;
    g_rgb_reg[1].hz = 2 * 10;

	UserTask_loop();
}

static void UserTask_timerCallback(void)
{
}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len)
{
    if((reg == CSReg_0) && (len == sizeof(led_tape_t)))
    {
    	g_rgb_reg[0] = *((const led_tape_t*)data);
        return CSTYPE_TRUE;
    }
    if((reg == CSReg_1) && (len == sizeof(led_tape_t)))
    {
    	g_rgb_reg[1] = *((const led_tape_t*)data);
        return CSTYPE_TRUE;
    }
    return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}

static void UserTask_runLED(const std::array<rgb_t, LED1_N + LED2_N>* red_arr){
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
