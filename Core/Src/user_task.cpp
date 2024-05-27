#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "tim.h"
#include "math.h"

#define ENC_TIM ((&htim2))

extern "C" {

typedef struct{
    int32_t count; //4
    int16_t pulse_ms; // pulse per milli second //2
    uint8_t checksum;
}__attribute__((__packed__)) count_t;

uint8_t calc_checksum(const count_t* count){
    const uint8_t* data = (const uint8_t*)count;
    return data[0] + data[1] + data[2] + data[3] + data[4] + data[5];
}

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static int32_t g_befor_count;
static count_t g_count_reg;

static float g_filt_pulse_ms = 0;

static bool g_rst_flg;
static CSTimer_t g_tim;

void UserTask_setup(void)
{
    HAL_TIM_Encoder_Start(ENC_TIM, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(ENC_TIM, 0);
    g_befor_count = 0;

    g_filt_pulse_ms = 0;

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
        int32_t count = __HAL_TIM_GET_COUNTER(ENC_TIM);
        g_count_reg.count = count;
        g_filt_pulse_ms = (count - g_befor_count) * 0.1 + g_filt_pulse_ms * 0.9;
        g_count_reg.pulse_ms = (int16_t)roundf(g_filt_pulse_ms * 100);
        g_count_reg.checksum = calc_checksum(&g_count_reg);
        g_befor_count = count;
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
