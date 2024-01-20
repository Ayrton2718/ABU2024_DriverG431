#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "usart.h"

#define RS485_HUART ((&huart2))
#define AMT212B_ID 	(0x54)

extern "C" {

typedef struct{
    int16_t rot_count;
    uint16_t angle;
}__attribute__((__packed__)) count_t;

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);
static bool UserTask_checksum(uint8_t low_byte, uint8_t high_byte);

static bool g_rst_flg;

static count_t g_count_reg;
static uint8_t  g_id;

static CSTimer_t g_tim;
static CSTimer_t g_timeout;


void UserTask_setup(void)
{
	g_id = AMT212B_ID;
    g_count_reg.rot_count = 0;
    g_count_reg.angle = 0;

    g_rst_flg = false;

    CSTimer_start(&g_tim);
    CSTimer_start(&g_timeout);

    CSIo_bind(CSType_appid_AMT212B, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_tim);
    if(1000 < us)
    {
        CSTimer_start(&g_tim);

        uint8_t rx_data[2] = {0};
        if(HAL_UART_Transmit(RS485_HUART, &g_id, 1, 10) == HAL_OK)
        {
            if(HAL_UART_Receive(RS485_HUART, rx_data, 2, 10) == HAL_OK)
			{
				if(UserTask_checksum(rx_data[0], rx_data[1]))
				{
					uint16_t last_angle = g_count_reg.angle;

					g_count_reg.angle = ((rx_data[1] & 0x3F) << 8 | rx_data[0]);

					if(last_angle < g_count_reg.angle)
					{
						if(8192 < (g_count_reg.angle - last_angle))
						{
							g_count_reg.rot_count--;
						}
					}else{
						if(8192 < (last_angle - g_count_reg.angle))
						{
							g_count_reg.rot_count++;
						}
					}
                    CSTimer_start(&g_timeout);
				}else{
                    CSLed_err();
                }
			}else{
				CSLed_err();
			}
        }else{
			CSLed_err();
        }

        if(CSTimer_getMs(g_timeout) < 100)
        {
            CSIo_sendUser(CSReg_0, (const uint8_t*)&g_count_reg, sizeof(count_t));
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
