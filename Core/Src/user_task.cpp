#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "BH1745NUC.h"
#include "veml3328.h"

extern "C" {

typedef struct{
    int16_t red;
    int16_t green;
    int16_t blue;
    int16_t ir;
}__attribute__((__packed__)) colors_t;

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static colors_t g_colors_reg;
static BH1745NUC* g_nuc = NULL;

static bool g_rst_flg;
static CSTimer_t g_tim;

void UserTask_setup(void)
{
	HAL_Delay(100);

    g_colors_reg.red = 0;
    g_colors_reg.green = 0;
    g_colors_reg.blue = 0;
    g_colors_reg.ir = 0;

    g_nuc = new BH1745NUC(BH1745NUC_DEVICE_ADDRESS_39);
    while(g_nuc->init() != 0)
    {
        CSLed_err();
        HAL_Delay(10);
        g_nuc->clear_err();
    }

    g_rst_flg = false;
    
    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_COLOR, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(&g_tim);
    if(1000 < us)
    {
        unsigned short rgbc[4];
        byte rc = g_nuc->get_val(rgbc);
        if (rc == 0)
        {
            g_colors_reg.red = rgbc[0];
            g_colors_reg.green = rgbc[1];
            g_colors_reg.blue = rgbc[2];
            g_colors_reg.ir = rgbc[3];
            CSIo_sendUser(CSReg_0, (const uint8_t*)&g_colors_reg, sizeof(colors_t));

            CSTimer_start(&g_tim);
        }else{
            CSLed_err();
            g_nuc->clear_err();
            
            g_nuc->init();
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
    return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}
