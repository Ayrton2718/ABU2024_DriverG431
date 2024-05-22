#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include <array>

extern "C" {

typedef struct{
    int16_t red;
    int16_t blue;
    int16_t clear;
    uint16_t elapsed_ms;
}__attribute__((__packed__)) colors_t;

typedef struct{
    uint8_t list1;
}__attribute__((__packed__)) switch_t;

inline bool get_sw(uint8_t* list, int index){
    if (index < 0 || index > 7) {
        return false;
    }
    return (*list >> index) & 1;
}

// 指定されたインデックスにbool値を設定する関数
inline void set_sw(uint8_t* list, int index, bool value) {
    if (index < 0 || index > 7) {
        return;
    }
    if (value) {
        *list |= (1 << index); // 指定されたビットを1に設定
    } else {
        *list &= ~(1 << index); // 指定されたビットを0に設定
    }
}

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static switch_t g_sw_reg;

static bool g_rst_flg;
static CSTimer_t g_send_tim;

static std::array<std::pair<GPIO_TypeDef*, uint16_t >, 4> g_sw_pins;


void UserTask_setup(void)
{
	HAL_Delay(20);

    g_sw_pins[0] = std::make_pair(SW1_GPIO_Port, SW1_Pin);
    g_sw_pins[1] = std::make_pair(SW2_GPIO_Port, SW2_Pin);
    g_sw_pins[2] = std::make_pair(SW3_GPIO_Port, SW3_Pin);
    g_sw_pins[3] = std::make_pair(SW4_GPIO_Port, SW4_Pin);

    g_rst_flg = false;
    
    CSTimer_start(&g_send_tim);
    CSIo_bind(CSType_appid_COLOR, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_send_tim);
    if(10000 < us)
    {
        CSTimer_start(&g_send_tim);

		// Switches
        for(size_t i = 0; i < g_sw_pins.size(); i++)
		{
			if(HAL_GPIO_ReadPin(g_sw_pins[i].first, g_sw_pins[i].second) == GPIO_PIN_SET){
				set_sw(&g_sw_reg.list1, i, false);
			}else{
				set_sw(&g_sw_reg.list1, i, true);
			}
		}
		CSIo_sendUser(CSReg_1, (const uint8_t*)&g_sw_reg, sizeof(switch_t));
    }

    if(g_rst_flg){
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
