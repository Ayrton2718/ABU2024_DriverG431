#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "veml3328.hpp"

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
static uint16_t g_err_count[4];

static bool g_rst_flg;
static CSTimer_t g_tim;

void UserTask_setup(void)
{
	HAL_Delay(20);

    g_colors_reg.red = 0;
    g_colors_reg.green = 0;
    g_colors_reg.blue = 0;
    g_colors_reg.ir = 0;

    g_err_count[0] = 0;
    g_err_count[1] = 0;
    g_err_count[2] = 0;
    g_err_count[3] = 0;

    for(uint16_t i = 0; i < 200; i++)
    {
        if(veml::begin() == HAL_OK)
        {
            break;
        }

        if(40 < i)
        {
            CSLed_err();
        }
        HAL_Delay(1);
    }

    g_rst_flg = false;
    
    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_COLOR, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_tim);
    if(50000 < us)
    {
        CSTimer_start(&g_tim);

        int16_t red, green, blue, ir;
		if(veml::getRed(&red) == HAL_OK){
            g_colors_reg.red = red;
            g_err_count[0] = 0;
        }else{
            g_err_count[0]++;
            if(4 < g_err_count[0]){
                CSLed_err();
            }
        }
        
        if(veml::getGreen(&green) == HAL_OK){
            g_colors_reg.green = green;
            g_err_count[1] = 0;
        }else{
            g_err_count[1]++;
            if(4 < g_err_count[1]){
                CSLed_err();
            }
        }

        if(veml::getBlue(&blue) == HAL_OK){
            g_colors_reg.blue = blue;
            g_err_count[2] = 0;
        }else{
            g_err_count[2]++;
            if(4 < g_err_count[2]){
                CSLed_err();
            }
        }

        if(veml::getIR(&ir) == HAL_OK){
            g_colors_reg.ir = ir;
            g_err_count[3] = 0;
        }else{
            g_err_count[3]++;
            if(4 < g_err_count[3]){
                CSLed_err();
            }
        }

		CSIo_sendUser(CSReg_0, (const uint8_t*)&g_colors_reg, sizeof(colors_t));

        if(20 * 4 < g_err_count[0] + g_err_count[1] + g_err_count[2] + g_err_count[3]){
            veml::begin();
        }
    }

    if(g_rst_flg)
    {
        g_rst_flg = 0;
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
