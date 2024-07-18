#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "sh2/Adafruit_BNO08x.h"

#include <math.h>

extern "C" {

typedef struct{
	int16_t gyro;
	int16_t angle;
	int8_t spin_count;
	int8_t acc_angle;
    uint16_t acc;
}__attribute__((__packed__)) yaw_t;

}

static Adafruit_BNO08x  g_bno08x;

static yaw_t  g_yaw_reg;

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static inline void sensorToYaw(const sh2_GyroIntegratedRV_t* sensor, yaw_t* rpy);
static inline void sensorToAcc(const sh2_Accelerometer_t* sensor, yaw_t* rpy);

static bool g_rst_flg;
static CSTimer_t g_tim;
static CSTimer_t g_timeout_tim;

static uint32_t g_count1 = 0;
static uint32_t g_count2 = 0;
static uint32_t g_us = 0;

void UserTask_setup(void)
{
    HAL_Delay(100);

    g_yaw_reg.gyro = 0;
    g_yaw_reg.angle = 0;
    g_yaw_reg.spin_count = 0;
    g_yaw_reg.acc_angle = 0;
    g_yaw_reg.acc = 0;

    // Try to initialize!
    bool is_success;

    is_success = false;
    for (size_t i = 0; i < 20; i++) {
        if(g_bno08x.begin_I2C() == true){
            is_success = true;
            break;
        }
        CSLed_err();
        HAL_Delay(10);
    }
    if(is_success == false){
        NVIC_SystemReset();
    }
    HAL_Delay(10);

    is_success = false;
    for (size_t i = 0; i < 20; i++) {
        if(g_bno08x.enableReport(SH2_LINEAR_ACCELERATION, 5000) == true){
            is_success = true;
            break;
        }
        CSLed_err();
        HAL_Delay(10);
    }
    if(is_success == false){
        NVIC_SystemReset();
    }
    HAL_Delay(10);

    is_success = false;
    for (size_t i = 0; i < 20; i++) {
        if(g_bno08x.enableReport(SH2_GYRO_INTEGRATED_RV, 5000) == true){
            is_success = true;
            break;
        }
        CSLed_err();
        HAL_Delay(10);
    }
    if(is_success == false){
        NVIC_SystemReset();
    }
    HAL_Delay(10);

    g_rst_flg = false;
    
    CSTimer_start(&g_tim);
    CSTimer_start(&g_timeout_tim);
    CSIo_bind(CSType_appid_UNKNOWN, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_tim);
    if(5000 < us)
    {
        CSTimer_start(&g_tim);
        g_us = us;

        bool is_success[2] = {false, false};
        for(size_t i = 0; i < 2; i++){
            sh2_SensorValue_t sensorValue;
            if(g_bno08x.getSensorEvent(&sensorValue)){
                switch (sensorValue.sensorId) {
                    case SH2_GYRO_INTEGRATED_RV:
                        sensorToYaw(&sensorValue.un.gyroIntegratedRV, &g_yaw_reg);
                        is_success[0] = true;
                        CSTimer_start(&g_timeout_tim);
                        break;
                    case SH2_LINEAR_ACCELERATION:
                        sensorToAcc(&sensorValue.un.linearAcceleration, &g_yaw_reg);
                        is_success[1] = true;
                        break;
                    default:
                        CSLed_err();
                        break;
                }
            }
            CSTimer_delayUs(100);
        }

        if((is_success[0]) == false){
            g_count1++;
        }

        if((is_success[1]) == false){
			g_count2++;
		}

        uint32_t ms = CSTimer_getMs(g_timeout_tim);
        if(ms < 100)
        {
            CSIo_sendUser(CSReg_0, (const uint8_t*)&g_yaw_reg, sizeof(yaw_t));
            CSLed_err();
        }

        if(g_bno08x.wasReset()) {
            for(size_t i = 0; i < 4; i++) {
                if(g_bno08x.enableReport(SH2_LINEAR_ACCELERATION, 5000) == true){
                    break;
                }
                CSLed_err();
                CSTimer_delayUs(100);
            }

            for(size_t i = 0; i < 4; i++) {
                if(g_bno08x.enableReport(SH2_GYRO_INTEGRATED_RV, 5000) == true){
                    break;
                }
                CSLed_err();
                CSTimer_delayUs(100);
            }
        }
        
        if(100 < g_count1){
            CSLed_err();
            g_count1 = 0;
        }

        if(100 < g_count2){
            CSLed_err();
            g_count2 = 0;
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

static inline void sensorToYaw(const sh2_GyroIntegratedRV_t* sensor, yaw_t* rpy)
{
    float qr = sensor->real;
    float qi = sensor->i;
    float qj = sensor->j;
    float qk = sensor->k;

    float sqr = qr * qr;
    float sqi = qi * qi;
    float sqj = qj * qj;
    float sqk = qk * qk;

    float now_yaw_f = atan2f(2.0 * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));

    int16_t now_yaw = static_cast<int16_t>(now_yaw_f * (180 / M_PI * 100));
    if((100 * 100) < rpy->angle && now_yaw < (-100 * 100))
    {
        rpy->spin_count++;
    }else if(rpy->angle < (-100 * 100) && (100 * 100) < now_yaw){
        rpy->spin_count--;
    }

    rpy->gyro = sensor->angVelZ * (180 / M_PI * 100);
    rpy->angle = now_yaw;
}

static inline void sensorToAcc(const sh2_Accelerometer_t* sensor, yaw_t* rpy)
{
	rpy->acc_angle = static_cast<int8_t>(atan2f(sensor->y, sensor->x) * (127 / M_PI));
	rpy->acc = static_cast<int16_t>(sqrtf(sensor->x*sensor->x + sensor->y*sensor->y) * 100);
}
