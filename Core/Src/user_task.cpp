#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

extern "C" {


}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static bool g_rst_flg;
static CSTimer_t g_tim;

void UserTask_setup(void)
{
    g_rst_flg = false;
    
    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_UNKNOWN, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(&g_tim);
    if(1000 < us)
    {
        CSTimer_start(&g_tim);
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
    return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}
