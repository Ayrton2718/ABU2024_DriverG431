#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "usart.h"

#define RS485_HUART ((&huart2))
#define AMT212B_ID 	(0x54)

extern "C" {

typedef struct{
    int16_t rot_count;
    uint16_t angle;
    uint8_t checksum;
}__attribute__((__packed__)) count_t;

uint8_t calc_checksum(count_t count){
    uint8_t* data = (uint8_t*)&count;
    return data[0] + data[1] + data[2] + data[3];
}

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);
static bool UserTask_checksum(uint8_t low_byte, uint8_t high_byte);

static bool g_rst_flg;

volatile static count_t g_count_reg;
static uint8_t  g_id;

static CSTimer_t g_tim;
static CSTimer_t g_send_tim;
static CSTimer_t g_timeout;

uint8_t g_rx_data[2] = {0};

void UserTask_setup(void)
{
	g_id = AMT212B_ID;

    g_count_reg.rot_count = 0;
    g_count_reg.angle = 0;

    g_rst_flg = false;

    CSTimer_start(&g_tim);
    CSTimer_start(&g_send_tim);
    CSTimer_start(&g_timeout);

    CSIo_bind(CSType_appid_AMT212B, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_tim);
    if(300 < us)
    {
        CSTimer_start(&g_tim);

        g_rx_data[0] = 0;
        g_rx_data[1] = 0;
        if(HAL_UART_Receive_IT(RS485_HUART, g_rx_data, 2) != HAL_OK)
        {
            CSLed_err();
        }
        if(HAL_UART_Transmit(RS485_HUART, &g_id, 1, 10) != HAL_OK)
        {
            CSLed_err();
        }
    }

    if(1 < CSTimer_getMs(g_send_tim))
    {
        CSTimer_start(&g_send_tim);

        if(CSTimer_getMs(g_timeout) < 100)
        {
            count_t reg;
            reg.angle = g_count_reg.angle;
            reg.rot_count = g_count_reg.rot_count;
            reg.checksum = calc_checksum(reg);
            CSIo_sendUser(CSReg_0, (const uint8_t*)&reg, sizeof(count_t));
        }else{
            CSLed_err();
        }
    }

    if(g_rst_flg)
    {
        g_count_reg.rot_count = 0;
        g_count_reg.angle = 0;
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

static bool UserTask_checksum(uint8_t low_byte, uint8_t high_byte) {
    auto l = [&](uint8_t i) { return (bool)((low_byte >> i) & 0x01); };
    auto h = [&](uint8_t i) { return (bool)((high_byte >> i) & 0x01); };
    bool k1 = !(h(5) ^ h(3) ^ h(1) ^ l(7) ^ l(5) ^ l(3) ^ l(1));
    bool k0 = !(h(4) ^ h(2) ^ h(0) ^ l(6) ^ l(4) ^ l(2) ^ l(0));
    return (k1 == h(7)) && (k0 == h(6));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == RS485_HUART->Instance)
	{
        if(UserTask_checksum(g_rx_data[0], g_rx_data[1]))
        {
            uint16_t last_angle = g_count_reg.angle;

            uint16_t now_angle = ((g_rx_data[1] & 0x3F) << 8 | g_rx_data[0]);

            if(last_angle < now_angle)
            {
                if(8192 < (now_angle - last_angle))
                {
                    g_count_reg.rot_count--;
                }
            }else{
                if(8192 < (last_angle - now_angle))
                {
                    g_count_reg.rot_count++;
                }
            }

            g_count_reg.angle = now_angle;
            CSTimer_start(&g_timeout);
        }else{
            CSLed_err();
        }
	}
}
