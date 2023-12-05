#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_timerCallback(void);

static CSTimer_t g_tim;

typedef struct{
    uint32_t count;
}__attribute__((__packed__)) count_t;


void UserTask_setup(void)
{
    CSTimer_start(&g_tim);

    // CSIo_bind(CSType_appid_AMT212B, UserTask_canCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(&g_tim);
    if(1000 < us)
    {
        CSTimer_start(&g_tim);

        count_t count;
        count.count = 100;

        // CSIo_sendUser(CSReg_0, (const uint8_t*)&count, sizeof(count_t));
    }
}

void UserTask_unsafeLoop(void)
{
    UserTask_loop();
}

void UserTask_timerCallback(void)
{
}

CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len)
{
    if(CSReg_0 == reg)
    {

    }else{

    }
}
