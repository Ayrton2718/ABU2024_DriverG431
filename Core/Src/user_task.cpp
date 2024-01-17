#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "tim.h"

#define ENC_TIM ((&htim2))

extern "C" {

typedef struct{
    int32_t count;
}__attribute__((__packed__)) count_t;

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static count_t g_count_reg;

static bool g_rst_flg;
static CSTimer_t g_tim;

void UserTask_setup(void)
{
    HAL_TIM_Encoder_Start(ENC_TIM, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(ENC_TIM, 0);

    g_rst_flg = false;

    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_AMT102, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_tim);
    if(1000 < us)
    {
        g_count_reg.count = __HAL_TIM_GET_COUNTER(ENC_TIM);
        CSIo_sendUser(CSReg_0, (const uint8_t*)&g_count_reg, sizeof(count_t));
        CSTimer_start(&g_tim);
    }

    if(g_rst_flg)
    {
        __HAL_TIM_SET_COUNTER(ENC_TIM, 0);
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
    return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}
