#include "user_task.h"
#include "can_smbus/can_smbus.hpp"

#include "sh2/Adafruit_BNO08x.h"

#include <math.h>

extern "C" {

typedef struct{
	int16_t gyro;
	int16_t angle;
	int16_t spin_count;
}__attribute__((__packed__)) yaw_t;

}

static Adafruit_BNO08x  g_bno08x;

static yaw_t  g_yaw_reg;

static sh2_SensorValue_t sensorValue;

static CSType_bool_t UserTask_canCallback(CSReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static inline void quaternionToEulerGI(const sh2_GyroIntegratedRV_t* rotational_vector, yaw_t* rpy);

static void set_sensor_reports(void);

static bool g_rst_flg;
static CSTimer_t g_tim;

void UserTask_setup(void)
{
    g_yaw_reg.gyro = 0;
    g_yaw_reg.angle = 0;
    g_yaw_reg.spin_count = 0;

    // Try to initialize!
    if (!g_bno08x.begin_I2C()) {
        CSLed_err();
        HAL_Delay(10);
    }

    set_sensor_reports();

    g_rst_flg = false;
    
    CSTimer_start(&g_tim);
    CSIo_bind(CSType_appid_UNKNOWN, UserTask_canCallback, UserTask_resetCallback);
    CSTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CSTimer_getUs(g_tim);
    if(5000 < us)
    {
        CSTimer_start(&g_tim);

//        sh2_SensorValue_t sensorValue;

        if (g_bno08x.getSensorEvent(&sensorValue)) {
            quaternionToEulerGI(&sensorValue.un.gyroIntegratedRV, &g_yaw_reg);
        }else{
            CSLed_err();
        }

        if (g_bno08x.wasReset()) {
            set_sensor_reports();
            CSLed_err();
        }

        CSIo_sendUser(CSReg_0, (const uint8_t*)&g_yaw_reg, sizeof(yaw_t));
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

static void set_sensor_reports(void){
    if (!g_bno08x.enableReport(SH2_GYRO_INTEGRATED_RV, 2000)){
        CSLed_err();
    }
}

static inline void quaternionToEulerGI(const sh2_GyroIntegratedRV_t* rotational_vector, yaw_t* rpy){
    float qr = rotational_vector->real;
    float qi = rotational_vector->i;
    float qj = rotational_vector->j;
    float qk = rotational_vector->k;

    float sqr = qr * qr;
    float sqi = qi * qi;
    float sqj = qj * qj;
    float sqk = qk * qk;

    // rpy->roll = atan2(2.0 * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr));
    // rpy->pitch = asin(-2.0 * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr));
    float now_yaw_f = atan2f(2.0 * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));

    int16_t now_yaw = now_yaw_f * (180 / M_PI * 100);
    if((100 * 100) < rpy->angle && now_yaw < (-100 * 100))
    {
        rpy->spin_count++;
    }else if(rpy->angle < (-100 * 100) && (100 * 100) < now_yaw){
        rpy->spin_count--;
    }

    rpy->gyro = rotational_vector->angVelZ * (180 / M_PI * 100);
    rpy->angle = now_yaw;
}
