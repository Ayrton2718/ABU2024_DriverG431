#include "user_task.h"
#include "can_smbus/can_smbus.hpp"
#include "Tri_Func_Filter.hpp"

#include <array>

extern "C" {

typedef struct{
    uint8_t list1;
}__attribute__((__packed__)) switch_t;

typedef struct{
	uint16_t count1;
	uint16_t count2;
	uint16_t count3;
	uint16_t count4;
}__attribute__((__packed__)) count_t;


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
static CSTimer_t g_tim;

static count_t g_count;

static std::array<std::pair<GPIO_TypeDef*, uint16_t >, 4> g_sw_pins;

static std::array<tut_mec_filter::Cos_Filter, 4> g_sw_filter;
static std::array<bool, 4> g_sw_data;
static std::array<bool, 4> g_sw_click;
static std::array<uint16_t, 4> g_sw_click_counter;
const float ttl_h = 0.5;
const float ttl_l = 0.5;
const float cycle_time = 0.001;

void UserTask_setup(void)
{
	HAL_Delay(20);
    g_rst_flg = false;

    g_sw_pins[0] = std::make_pair(SW1_GPIO_Port, SW1_Pin);
    g_sw_pins[1] = std::make_pair(SW2_GPIO_Port, SW2_Pin);
    g_sw_pins[2] = std::make_pair(SW3_GPIO_Port, SW3_Pin);
    g_sw_pins[3] = std::make_pair(SW4_GPIO_Port, SW4_Pin);

    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_SWITCH, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);

    for(size_t i; i < g_sw_data.size(); i++){
    	g_sw_data[i] = false;
    	g_sw_click[i] = false;
    	g_sw_click_counter[i] = 0;
    	g_sw_filter[i].init(0.004, 0.001);
    }
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_tim);
    if(1000 < us)
    {
        CSTimer_start(&g_tim);
        float sec = us * 0.000001;

        for(size_t i = 0; i < g_sw_pins.size(); i++)
        {
            if(HAL_GPIO_ReadPin(g_sw_pins[i].first, g_sw_pins[i].second) == GPIO_PIN_SET){
                set_sw(&g_sw_reg.list1, i, false);
            }else{
                set_sw(&g_sw_reg.list1, i, true);
            }

            // ここからR2専用
            float filter_output = g_sw_filter[i].Cal_Data((float)get_sw(&g_sw_reg.list1, i), sec);
            if(ttl_h < filter_output && !g_sw_data[i]){
            	g_sw_data[i] = true;
            	g_sw_click[i] = true;
            }
            else if(filter_output < ttl_l && g_sw_data[i]){
            	g_sw_data[i] = false;
            	g_sw_click[i] = false;
            }
            else{
            	g_sw_click[i] = false;
            }

            if(g_sw_click[i]){
            	g_sw_click_counter[i]++;
            	g_sw_click[i] = false;
            }

        }

        g_count.count1 = g_sw_click_counter[0];
        g_count.count2 = g_sw_click_counter[1];
        g_count.count3 = g_sw_click_counter[2];
        g_count.count4 = g_sw_click_counter[3];

		CSIo_sendUser(CSReg_0, (const uint8_t*)&g_sw_reg, sizeof(switch_t));
		CSIo_sendUser(CSReg_1, (const uint8_t*)&g_count, sizeof(count_t));
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

