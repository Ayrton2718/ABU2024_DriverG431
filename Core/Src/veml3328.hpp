#ifndef VEML3328_H
#define VEML3328_H

#include <main.h>
#include <stdint.h>
#include <string>

#include <i2c.h>
#include <string.h>

#define I2C_HANDLE ((&hi2c2))
#define VEML_ADDRESS (I2C_ADDRESS_DEFAULT << 1)
#define VEML_I2C_DELAY  (2)

#define SerialDebug Serial3 // Cellular Mini
//#define __DEBUG

#define I2C_ADDRESS_DEFAULT 0x10
#define POINTER_CONFIG      0x00 // VEML3328 configuration register
#define POINTER_CLEAR       0x04 // Red channel pointer
#define POINTER_RED         0x05 // Blue channel pointer
#define POINTER_GREEN       0x06 // Green channel pointer
#define POINTER_BLUE        0x07 // Blue channel pointer
#define POINTER_IR          0x08 // Infrared (IR) channel pointer
#define POINTER_DEVICE_ID   0x0C // Device ID = 0x28

typedef enum 
{ 
	dg_x1 = 0x00, 
	dg_x2 = 0x01, 
	dg_x4 = 0x02 
} DG_t;

typedef enum 
{
	gain_x1_2 = 0x03,
	gain_x1 = 0x00,
	gain_x2 = 0x01,
	gain_x4 = 0x02
} gain_t;

typedef enum
{
	time_50 = 0x00,
	time_100 = 0x01,
	time_200 = 0x02,
	time_400 = 0x03
} int_time_t;

namespace veml{

inline HAL_StatusTypeDef regWrite(uint8_t reg_ptr, uint16_t data) {
    uint8_t buff[2];
    buff[0] = data & 0xFF;
    buff[1] = data >> 8;

    return HAL_I2C_Mem_Write(I2C_HANDLE, VEML_ADDRESS, reg_ptr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)buff, 2, VEML_I2C_DELAY);
}

inline HAL_StatusTypeDef regRead(uint8_t reg_ptr, uint16_t* data) {
    /* Variables */
    unsigned char rx_data[2] = {0};

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(I2C_HANDLE, VEML_ADDRESS, reg_ptr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)rx_data, 2, VEML_I2C_DELAY);
    *data = (rx_data[1] << 8) | rx_data[0];
    return status;
}

inline HAL_StatusTypeDef wake(void) {
    if(regWrite(POINTER_CONFIG, ((0 << 15) & (0 << 0))) != HAL_OK){
        return HAL_ERROR;
    }

    /* Check shutdown bits */
    uint16_t reg;
    regRead(POINTER_CONFIG, &reg);
    if ((reg & (1 << 15)) && (reg & (1 << 0))) {
#ifdef __DEBUG
        this->log_out("Error: shutdown bits set");
#endif
        return HAL_ERROR;
    }

    return HAL_OK;
}

inline HAL_StatusTypeDef begin(void) {
    return wake();
}

inline HAL_StatusTypeDef shutdown(void) {
    /* Set shutdown bits SD1/SD0 */
    return regWrite(POINTER_CONFIG, ((1 << 15) | (1 << 0)));
}

inline HAL_StatusTypeDef getRed(int16_t* data) {
    return regRead(POINTER_RED, (uint16_t*)data);
}

inline HAL_StatusTypeDef getGreen(int16_t* data) {
    return regRead(POINTER_GREEN, (uint16_t*)data);
}

inline HAL_StatusTypeDef getBlue(int16_t* data) {
    return regRead(POINTER_BLUE, (uint16_t*)data); 
}

inline HAL_StatusTypeDef getIR(int16_t* data) {
    return regRead(POINTER_IR, (uint16_t*)data);
}

inline HAL_StatusTypeDef getClear(int16_t* data) {
    return regRead(POINTER_CLEAR, (uint16_t*)data);
}

inline HAL_StatusTypeDef deviceID(uint16_t* data) {
    HAL_StatusTypeDef status = regRead(POINTER_DEVICE_ID, data);
    *data = *data & 0xFF;
    return status; // LSB data
}

inline HAL_StatusTypeDef rbShutdown(void) {
    if(regWrite(POINTER_CONFIG, (1 << 14)) != HAL_OK){
        return HAL_ERROR;
    }

    /* Check shutdown bit */
    uint16_t reg;
    regRead(POINTER_CONFIG, &reg);

    if (!(reg & (1 << 14))) {
#ifdef __DEBUG
        this->log_out("Error: shutdown bit not set");
#endif
        return HAL_ERROR;
    }

    return HAL_OK;
}

inline HAL_StatusTypeDef rbWakeup(void) {
    if(regWrite(POINTER_CONFIG, (0 << 14)) != HAL_OK){
        return HAL_ERROR;
    }
    /* Check shutdown bit */
    uint16_t reg;
    regRead(POINTER_CONFIG, &reg);
    if (reg & (1 << 14)) {
#ifdef __DEBUG
        this->log_out("Error: shutdown bit set");
#endif
        return HAL_ERROR;
    }

    return HAL_OK;
}

inline HAL_StatusTypeDef setDG(DG_t val) {
    if(regWrite(POINTER_CONFIG, (uint16_t)val) != HAL_OK){
        return HAL_ERROR;
    }

    /* Check user val */
    uint16_t reg;
    regRead(POINTER_CONFIG, &reg);
    if (!(reg & val)) {
#ifdef __DEBUG
        this->log_out("Error: DG not set");
#endif
        return HAL_ERROR;
    }

    return HAL_OK;
}

inline HAL_StatusTypeDef setGain(gain_t val) {
    if(regWrite(POINTER_CONFIG, (uint16_t)val) != HAL_OK){
        return HAL_ERROR;
    }

    /* Check user val */
    uint16_t reg;
    regRead(POINTER_CONFIG, &reg);
    if (!(reg & val)) {
#ifdef __DEBUG
        this->log_out("Error: gain not set");
#endif
        return HAL_ERROR;
    }

    return HAL_OK;
}

inline HAL_StatusTypeDef setSensitivity(bool high_low_sens) {
    /* Variables */
    if (high_low_sens) {
        if(regWrite(POINTER_CONFIG, (1 << 6)) != HAL_OK){
            return HAL_ERROR;
        }

        uint16_t reg;
        regRead(POINTER_CONFIG, &reg);
        if (reg & (0 << 6)) {
#ifdef __DEBUG
            this->log_out("Error: gain not set");
#endif
            return HAL_ERROR;
        }
    } else {
        if(regWrite(POINTER_CONFIG, (0 << 6)) != HAL_OK){
            return HAL_ERROR;
        }

        uint16_t reg;
        regRead(POINTER_CONFIG, &reg);
        if (reg & (1 << 6)) {
#ifdef __DEBUG
            this->log_out("Error: gain not set");
#endif
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

inline HAL_StatusTypeDef setIntTime(int_time_t time) {
    if(regWrite(POINTER_CONFIG, (uint16_t)time) != HAL_OK){
        return HAL_ERROR;
    }

    /* Check user val */
    uint16_t reg;
    regRead(POINTER_CONFIG, &reg);
    if (!(reg & time)) {
#ifdef __DEBUG
        this->log_out("Error: gain not set");
#endif
        return HAL_ERROR;
    }

    return HAL_OK;
}


}

#endif