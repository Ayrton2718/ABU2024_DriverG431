#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "usart.h"

extern "C" {

typedef struct{
    int16_t rot_count;
    uint16_t angle;
}__attribute__((__packed__)) count_t;

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_timerCallback(void);
static bool UserTask_checksum(uint8_t low_byte, uint8_t high_byte);

static count_t g_count_reg;

static uint8_t  g_id = 0x54;

static CSTimer_t g_tim;


void UserTask_setup(void)
{
    bool is_available = false;
    g_count_reg.rot_count = 0;
    g_count_reg.angle = 0;

    uint8_t rx_data[2] = {0};
    HAL_UART_Transmit(&huart2, &g_id, 1, 100);
    if(HAL_UART_Receive(&huart2, rx_data, 2, 100) == HAL_OK)
    {
        if(UserTask_checksum(rx_data[0], rx_data[1]))
        {
            uint16_t data = ((rx_data[1] & 0x3F) << 8 | rx_data[0]);
            g_count_reg.angle = data;
            is_available = true;
        }
    }

    if(is_available == false)
    {
        CSLed_err();
    }

    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_AMT212B, UserTask_canCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(&g_tim);
    if(1000 < us)
    {
        CSTimer_start(&g_tim);

        uint8_t rx_data[2] = {0};
        if(HAL_UART_Transmit(&huart2, &g_id, 1, 10) == HAL_OK)
        {
            if(HAL_UART_Receive(&huart2, rx_data, 2, 10) == HAL_OK)
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
				}else{
                    CSLed_err();
                }
			}else{
				CSLed_err();
			}
        }

        CSIo_sendUser(CSReg_0, (const uint8_t*)&g_count_reg, sizeof(count_t));
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
    return CSTYPE_FALSE;
}


static bool UserTask_checksum(uint8_t low_byte, uint8_t high_byte) {
    auto l = [&](uint8_t i) { return (bool)((low_byte >> i) & 0x01); };
    auto h = [&](uint8_t i) { return (bool)((high_byte >> i) & 0x01); };
    bool k1 = !(h(5) ^ h(3) ^ h(1) ^ l(7) ^ l(5) ^ l(3) ^ l(1));
    bool k0 = !(h(4) ^ h(2) ^ h(0) ^ l(6) ^ l(4) ^ l(2) ^ l(0));
    return (k1 == h(7)) && (k0 == h(6));
}
