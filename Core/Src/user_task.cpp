#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "usart.h"

#define UART_HANDLE ((&huart2))

 #define ENABLE_ANGLE_OUT
//#define ENABLE_GYRO_OUT
// #define ENABLE_ACC_OUT

extern "C" {

typedef struct{
    int16_t roll;   // 1/100 [degree]
    int16_t pitch;  // 1/100 [degree]
    int16_t yaw;    // 1/100 [degree]
}__attribute__((__packed__)) angle_t;

typedef struct{
    int16_t roll;   // 1/100 [degree]
    int16_t pitch;  // 1/100 [degree]
    int16_t yaw;    // 1/100 [degree]
}__attribute__((__packed__)) gyro_t;

typedef struct{
    int16_t x;      // 1/1000 [g]
    int16_t y;      // 1/1000 [g]
    int16_t z;      // 1/1000 [g]
}__attribute__((__packed__)) acc_t;

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static angle_t  g_angle_reg;
static gyro_t   g_gyro_reg;
static acc_t    g_acc_reg;

static bool     g_is_connecting;

static uint8_t  g_last_index;
static uint8_t  g_data_flg;
static uint8_t  g_data[2][26];

static uint8_t  g_rx_dma[52];
static uint8_t  g_befor_index[2];

static bool g_rst_flg;
static CSTimer_t g_tim;
static CSTimer_t g_data_tim;


void UserTask_setup(void)
{
    g_rst_flg = false;

    g_angle_reg.roll = 0;
    g_angle_reg.pitch = 0;
    g_angle_reg.yaw = 0;

    g_gyro_reg.roll = 0;
    g_gyro_reg.pitch = 0;
    g_gyro_reg.yaw = 0;

    g_acc_reg.x = 0;
    g_acc_reg.y = 0;
    g_acc_reg.z = 0;

    g_is_connecting = false;

    g_last_index = 0;
    g_data_flg = 0;
    memset(&g_data[0], 0x00, sizeof(uint8_t) * 2);
    memset(&g_data[1], 0x00, sizeof(uint8_t) * 2);

    memset(g_rx_dma, 0x00, sizeof(g_rx_dma));
    memset(g_befor_index, 0x00, sizeof(g_befor_index));

    HAL_Delay(500);
    HAL_UART_Receive_DMA(UART_HANDLE, g_rx_dma, sizeof(g_rx_dma));
    
    g_rst_flg = false;

    CSTimer_start(&g_tim);
    CSTimer_start(&g_data_tim);
    CSIo_bind(CSType_appid_UNKNOWN, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    if(50 < CSTimer_getMs(g_data_tim))
    {
        CSLed_err();
    }

    if(1000 < CSTimer_getUs(g_tim))
    {
        uint8_t data_flg = (g_data_flg + 1) % 2;
        g_angle_reg.roll = g_data[data_flg][4] << 8 | g_data[data_flg][3];
        g_angle_reg.pitch = (g_data[data_flg][6] << 8 | g_data[data_flg][5]);
        g_angle_reg.yaw = (g_data[data_flg][8] << 8 | g_data[data_flg][7]);
        g_gyro_reg.roll = (g_data[data_flg][10] << 8 | g_data[data_flg][9]);
        g_gyro_reg.pitch = (g_data[data_flg][12] << 8 | g_data[data_flg][11]);
        g_gyro_reg.yaw = (g_data[data_flg][14] << 8 | g_data[data_flg][13]);
        g_acc_reg.x = g_data[data_flg][16] << 8 | g_data[data_flg][15];
        g_acc_reg.y = g_data[data_flg][18] << 8 | g_data[data_flg][17];
        g_acc_reg.z = g_data[data_flg][20] << 8 | g_data[data_flg][19];

#ifdef ENABLE_ANGLE_OUT
        CSIo_sendUser(CSReg_0, (const uint8_t*)&g_angle_reg, sizeof(angle_t));
#endif /*ENABLE_ANGLE_OUT*/

#ifdef ENABLE_GYRO_OUT
        CSIo_sendUser(CSReg_1, (const uint8_t*)&g_gyro_reg, sizeof(gyro_t));
#endif /*ENABLE_GYRO_OUT*/

#ifdef ENABLE_ACC_OUT
        CSIo_sendUser(CSReg_2, (const uint8_t*)&g_acc_reg, sizeof(acc_t));
#endif /*ENABLE_ACC_OUT*/
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
	g_rst_flg = true;
}


static inline void USerTask_searchData(void)
{
    for(uint8_t start_i = g_last_index; start_i < 52 + g_last_index; start_i++)
    {
        if(g_rx_dma[start_i % 52] == 0xA6 && g_rx_dma[(start_i + 1) % 52] == 0xA6)
        {
            uint8_t data_flg = (g_data_flg + 1) % 2;
            uint8_t checksum = 0;

            for (int j = 0; j < 26; j++) {
                g_data[data_flg][j] = g_rx_dma[(start_i + j) % 52];
                if (j != 0 && j != 1 && j != 25) {
                    checksum += g_data[data_flg][j];
                }
            }

            if(checksum == g_data[data_flg][25])
            {
                g_data_flg = data_flg;
                CSTimer_start(&g_data_tim);
            }

            g_last_index = start_i + 2;
            break;
        }
    }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == UART_HANDLE->Instance)
    {
        USerTask_searchData();
    }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == UART_HANDLE->Instance)
    {
        USerTask_searchData();
    }
}
