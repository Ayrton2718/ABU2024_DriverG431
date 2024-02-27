#include "veml3328.hpp"

#include <i2c.h>
#include <string.h>

#define I2C_HANDLE ((&hi2c2))
#define VEML_ADDRESS (I2C_ADDRESS_DEFAULT << 1)

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

/* Forward function delcarations */
HAL_StatusTypeDef regWrite(uint8_t reg_ptr, uint16_t data);
HAL_StatusTypeDef regRead(uint8_t reg_ptr, uint16_t* data);

/* Singleton instance. Used by rest of library */
VEMLClass Veml3328 = VEMLClass::instance();


HAL_StatusTypeDef VEMLClass::begin(void) {
    return Veml3328.wake();
}

HAL_StatusTypeDef VEMLClass::wake(void) {
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

HAL_StatusTypeDef VEMLClass::shutdown(void) {
    /* Set shutdown bits SD1/SD0 */
    return regWrite(POINTER_CONFIG, ((1 << 15) | (1 << 0)));
}

HAL_StatusTypeDef VEMLClass::getRed(int16_t* data) {
    return regRead(POINTER_RED, (uint16_t*)data);
}

HAL_StatusTypeDef VEMLClass::getGreen(int16_t* data) {
    return regRead(POINTER_GREEN, (uint16_t*)data);
}

HAL_StatusTypeDef VEMLClass::getBlue(int16_t* data) {
    return regRead(POINTER_BLUE, (uint16_t*)data); 
}

HAL_StatusTypeDef VEMLClass::getIR(int16_t* data) {
    return regRead(POINTER_IR, (uint16_t*)data);
}

HAL_StatusTypeDef VEMLClass::getClear(int16_t* data) {
    return regRead(POINTER_CLEAR, (uint16_t*)data);
}

HAL_StatusTypeDef VEMLClass::deviceID(uint16_t* data) {
    HAL_StatusTypeDef status = regRead(POINTER_DEVICE_ID, data);
    *data = *data & 0xFF;
    return status; // LSB data
}

HAL_StatusTypeDef VEMLClass::rbShutdown(void) {
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

HAL_StatusTypeDef VEMLClass::rbWakeup(void) {
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

HAL_StatusTypeDef VEMLClass::setDG(DG_t val) {
    if(regWrite(POINTER_CONFIG, val) != HAL_OK){
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

HAL_StatusTypeDef VEMLClass::setGain(gain_t val) {
    if(regWrite(POINTER_CONFIG, val) != HAL_OK){
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

HAL_StatusTypeDef VEMLClass::setSensitivity(bool high_low_sens) {
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

HAL_StatusTypeDef VEMLClass::setIntTime(int_time_t time) {
    if(regWrite(POINTER_CONFIG, time) != HAL_OK){
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

/**
 * @brief 16-bit write procedure
 *
 * @param reg_ptr Register pointer
 * @param data 16-bit data
 */
HAL_StatusTypeDef regWrite(uint8_t reg_ptr, uint16_t data) {
    uint8_t buff[2];
    buff[0] = data & 0xFF;
    buff[1] = data >> 8;

    return HAL_I2C_Mem_Write(I2C_HANDLE, VEML_ADDRESS, reg_ptr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)buff, 2, 5);
}

/**
 * @brief 16-bit read procedure
 *
 * @param reg_ptr Register pointer
 * @return uint16_t Returned data
 */
HAL_StatusTypeDef regRead(uint8_t reg_ptr, uint16_t* data) {
    /* Variables */
    unsigned char rx_data[2] = {0};

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(I2C_HANDLE, VEML_ADDRESS, reg_ptr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)rx_data, 2, 5);
    *data = (rx_data[1] << 8) | rx_data[0];
    return status;
}
