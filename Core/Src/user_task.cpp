#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "usart.h"
#include "math.h"

/*
R6093U
https://ip.festo-didactic.com/Infoportal/Robotino/document/gyro.pdf
*/

#define UART_HANDLE ((&huart2))

#define ENABLE_ANGLE_OUT
#define ENABLE_GYRO_OUT
// #define ENABLE_ACC_OUT

extern "C" {

/// @brief gyro：角速度、angle：角度（振り切れる）、spin_count：回転数
    typedef struct{
        int16_t gyro;
        int16_t angle;
        int8_t spin_count;
        int8_t acc_angle;
        uint16_t acc;
    }__attribute__((__packed__)) yaw_t;

}

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static yaw_t  g_yaw_reg;

static int16_t  g_befor_angle;

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

    g_yaw_reg.gyro = 0;    
    g_yaw_reg.angle = 0;
    g_yaw_reg.spin_count = 0;
    g_yaw_reg.acc_angle = 0;
    g_yaw_reg.acc = 0;

    g_befor_angle = 0;

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
    if(20 < CSTimer_getMs(g_data_tim))
    {
    	g_is_connecting = false;
        CSLed_err();
    }else{
    	g_is_connecting = true;
    }

    if((1000 < CSTimer_getUs(g_tim)) && g_is_connecting)
    {
        uint8_t data_flg = (g_data_flg + 1) % 2;
        g_yaw_reg.gyro = (g_data[data_flg][10] << 8 | g_data[data_flg][9]) * -1;
        g_yaw_reg.angle = ((g_data[data_flg][8] << 8 | g_data[data_flg][7])) * -1;
        float acc_x = (float)(g_data[data_flg][16] << 8 | g_data[data_flg][15]) / 1000.0f;
        float acc_y= (float)(g_data[data_flg][18] << 8 | g_data[data_flg][17]) / 1000.0f;
        g_yaw_reg.acc_angle = static_cast<int8_t>(atan2f(acc_y, acc_x) * (127 / M_PI));
        g_yaw_reg.acc = static_cast<int16_t>(sqrtf(acc_x*acc_x + acc_y*acc_y) * 100);

        if((100 * 100) < g_befor_angle && g_yaw_reg.angle < (-100 * 100))
        {
            g_yaw_reg.spin_count++;
        }else if(g_befor_angle < (-100 * 100) && (100 * 100) < g_yaw_reg.angle){
            g_yaw_reg.spin_count--;
        }
        g_befor_angle = g_yaw_reg.angle;

#ifdef ENABLE_ANGLE_OUT
        CSIo_sendUser(CSReg_0, (const uint8_t*)&g_yaw_reg, sizeof(yaw_t));
#endif /*ENABLE_ANGLE_OUT*/

        CSTimer_start(&g_tim);
    }

    if(g_rst_flg)
    {
        g_yaw_reg.gyro = 0;    
        g_yaw_reg.angle = 0;
        g_yaw_reg.spin_count = 0;
        g_yaw_reg.acc_angle = 0;
        g_yaw_reg.acc = 0;

        g_befor_angle = 0;

        g_is_connecting = false;

        g_last_index = 0;
        g_data_flg = 0;
        memset(&g_data[0], 0x00, sizeof(uint8_t) * 2);
        memset(&g_data[1], 0x00, sizeof(uint8_t) * 2);

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
    for(uint16_t start_i = g_last_index; start_i < 52 + g_last_index; start_i++)
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
                g_last_index = (start_i + 25 + 1) % 52;
                g_data_flg = data_flg;
                CSTimer_start(&g_data_tim);
                break;
            }
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
