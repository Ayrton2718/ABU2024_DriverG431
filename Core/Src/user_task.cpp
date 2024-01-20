#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#define PWM_HANDLE ((&htim2))

extern "C" {

typedef struct{
    uint8_t duty;
}__attribute__((__packed__)) sky_t;

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

// You have to change here to flipping buff.
static sky_t g_sky1;
static sky_t g_sky2;

static bool g_rst_flg;
static CSTimer_t g_tim;

void UserTask_setup(void)
{
    HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(PWM_HANDLE, TIM_CHANNEL_2);

    __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_1, 1000);
    __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_2, 1000);

    g_sky1.duty = 0;
    g_sky2.duty = 0;

    g_rst_flg = false;
    
    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_SKY_WALKER, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    if(g_rst_flg)
    {
        g_sky1.duty = 0;
        g_sky2.duty = 0;
        __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_1, 1000);
        __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_2, 1000);
        
        g_rst_flg = false;
    }
}

void UserTask_unsafeLoop(void)
{
    if(HAL_GPIO_ReadPin(BTN_ID_GPIO_Port, BTN_ID_Pin) == GPIO_PIN_RESET)
    {
        if(1000 < CSTimer_getMs(g_tim))
        {
            __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_1, 2000);
            __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_2, 2000);
            CSLed_err();
        }else{
            __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_1, 1000);
            __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_2, 1000);
        }
    }else{
        __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_1, 1000);
        __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_2, 1000);
        CSTimer_start(&g_tim);
    }
}

static void UserTask_timerCallback(void)
{
}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len)
{
    if((reg == CSReg_0) && (len == sizeof(sky_t)))
    {
        g_sky1 = *((const sky_t*)data);
        uint16_t set_duty = ((uint32_t)g_sky1.duty * 1000 / 255) + 1000;
        __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_1, set_duty);
        return CSTYPE_TRUE;
    }else if((reg == CSReg_1) && (len == sizeof(sky_t)))
    {
        g_sky2 = *((const sky_t*)data);
        uint16_t set_duty = ((uint32_t)g_sky2.duty * 1000 / 255) + 1000;
        __HAL_TIM_SET_COMPARE(PWM_HANDLE, TIM_CHANNEL_2, set_duty);
        return CSTYPE_TRUE;
    }
    return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}
