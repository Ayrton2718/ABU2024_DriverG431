#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "veml3328.hpp"

#define PWM1_HANDLE ((&htim2))
#define PWM1_CHANNEL (TIM_CHANNEL_1)
#define PWM2_HANDLE ((&htim3))
#define PWM2_CHANNEL (TIM_CHANNEL_4)

extern "C" {

typedef struct{
    int16_t red;
    int16_t green;
    int16_t blue;
    int16_t ir;
}__attribute__((__packed__)) colors_t;

typedef struct{
    uint8_t duty;
}__attribute__((__packed__)) sky_t;

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static colors_t g_colors_reg;
static uint16_t g_err_count[4];

// You have to change here to flipping buff.
static sky_t g_sky1;
static sky_t g_sky2;

static bool g_rst_flg;
static CSTimer_t g_tim;
static CSTimer_t g_sky_tim;

static uint8_t osijdfoijtmp;


void UserTask_setup(void)
{
   HAL_TIM_PWM_Start(PWM1_HANDLE, PWM1_CHANNEL);
   HAL_TIM_PWM_Start(PWM2_HANDLE, PWM2_CHANNEL);

   __HAL_TIM_SET_COMPARE(PWM1_HANDLE, PWM1_CHANNEL, 1000);
   __HAL_TIM_SET_COMPARE(PWM2_HANDLE, PWM2_CHANNEL, 1000);

    g_sky1.duty = 0;
    g_sky2.duty = 0;

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
        g_sky1.duty = 0;
        g_sky2.duty = 0;
       __HAL_TIM_SET_COMPARE(PWM1_HANDLE, PWM1_CHANNEL, 1000);
       __HAL_TIM_SET_COMPARE(PWM2_HANDLE, PWM2_CHANNEL, 1000);

        g_rst_flg = 0;
    }
}

void UserTask_unsafeLoop(void)
{
   if(HAL_GPIO_ReadPin(BTN_ID_GPIO_Port, BTN_ID_Pin) == GPIO_PIN_RESET)
   {
       if(1000 < CSTimer_getMs(g_sky_tim))
       {
           __HAL_TIM_SET_COMPARE(PWM1_HANDLE, PWM1_CHANNEL, 2000);
           __HAL_TIM_SET_COMPARE(PWM2_HANDLE, PWM2_CHANNEL, 2000);
           CSLed_err();
       }else{
           __HAL_TIM_SET_COMPARE(PWM1_HANDLE, PWM1_CHANNEL, 1000);
           __HAL_TIM_SET_COMPARE(PWM2_HANDLE, PWM2_CHANNEL, 1000);
       }
   }else{
       __HAL_TIM_SET_COMPARE(PWM1_HANDLE, PWM1_CHANNEL, 1000);
       __HAL_TIM_SET_COMPARE(PWM2_HANDLE, PWM2_CHANNEL, 1000);
       CSTimer_start(&g_sky_tim);
   }

    UserTask_loop();
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
       __HAL_TIM_SET_COMPARE(PWM1_HANDLE, PWM1_CHANNEL, set_duty);
        return CSTYPE_TRUE;
    }else if((reg == CSReg_1) && (len == sizeof(sky_t)))
    {
        g_sky2 = *((const sky_t*)data);
        uint16_t set_duty = ((uint32_t)g_sky2.duty * 1000 / 255) + 1000;
       __HAL_TIM_SET_COMPARE(PWM2_HANDLE, PWM2_CHANNEL, set_duty);
        return CSTYPE_TRUE;
    }
    return CSTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}
