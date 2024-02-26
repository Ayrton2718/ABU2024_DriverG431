#include "veml3328.h"

#include <i2c.h>
#include <string.h>

#define I2C_HANDLE ((&hi2c2))

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

/* Global variables */
static uint8_t I2C_ADDRESS;

/* Forward function delcarations */
void regWrite(uint8_t reg_ptr, uint16_t data);
uint16_t regRead(uint8_t reg_ptr);

/* Singleton instance. Used by rest of library */
VEMLClass Veml3328 = VEMLClass::instance();


uint8_t VEMLClass::begin(void) {
    I2C_ADDRESS = I2C_ADDRESS_DEFAULT;
    return Veml3328.wake();
}

uint8_t VEMLClass::begin(uint8_t address) {
    I2C_ADDRESS = address;
    return Veml3328.wake();
}

uint8_t VEMLClass::wake(void) {
    regWrite(POINTER_CONFIG,
             ((0 << 15) & (0 << 0))); // Set shutdown bits SD1/SD0

    /* Check shutdown bits */
    uint16_t reg = regRead(POINTER_CONFIG);
    if ((reg & (1 << 15)) && (reg & (1 << 0))) {
#ifdef __DEBUG
        this->log_out("Error: shutdown bits set");
#endif
        return 1;
    }

    return 0;
}

void VEMLClass::shutdown(void) {
    /* Set shutdown bits SD1/SD0 */
    regWrite(POINTER_CONFIG, ((1 << 15) | (1 << 0)));
}

int16_t VEMLClass::getRed(void) { return regRead(POINTER_RED); }

int16_t VEMLClass::getGreen(void) { return regRead(POINTER_GREEN); }

int16_t VEMLClass::getBlue(void) { return regRead(POINTER_BLUE); }

int16_t VEMLClass::getIR(void) { return regRead(POINTER_IR); }

int16_t VEMLClass::getClear(void) { return regRead(POINTER_CLEAR); }

uint16_t VEMLClass::deviceID(void) {
    return (regRead(POINTER_DEVICE_ID) & 0xFF); // LSB data
}

uint8_t VEMLClass::rbShutdown(void) {
    regWrite(POINTER_CONFIG, (1 << 14));

    /* Check shutdown bit */
    uint16_t reg = regRead(POINTER_CONFIG);
    if (!(reg & (1 << 14))) {
#ifdef __DEBUG
        this->log_out("Error: shutdown bit not set");
#endif
        return 1;
    }

    return 0;
}

uint8_t VEMLClass::rbWakeup(void) {
    regWrite(POINTER_CONFIG, (0 << 14));

    /* Check shutdown bit */
    uint16_t reg = regRead(POINTER_CONFIG);
    if (reg & (1 << 14)) {
#ifdef __DEBUG
        this->log_out("Error: shutdown bit set");
#endif
        return 1;
    }

    return 0;
}

uint8_t VEMLClass::setDG(DG_t val) {
    regWrite(POINTER_CONFIG, val);

    /* Check user val */
    uint16_t reg = regRead(POINTER_CONFIG);
    if (!(reg & val)) {
#ifdef __DEBUG
        this->log_out("Error: DG not set");
#endif
        return 1;
    }

    return 0;
}

uint8_t VEMLClass::setGain(gain_t val) {
    regWrite(POINTER_CONFIG, val);

    /* Check user val */
    uint16_t reg = regRead(POINTER_CONFIG);
    if (!(reg & val)) {
#ifdef __DEBUG
        this->log_out("Error: gain not set");
#endif
        return 1;
    }

    return 0;
}

uint8_t VEMLClass::setSensitivity(bool high_low_sens) {
    /* Variables */
    uint16_t reg;

    if (high_low_sens) {
        regWrite(POINTER_CONFIG, (1 << 6));

        reg = regRead(POINTER_CONFIG);
        if (reg & (0 << 6)) {
#ifdef __DEBUG
            this->log_out("Error: gain not set");
#endif
            return 1;
        }
    } else {
        regWrite(POINTER_CONFIG, (0 << 6));

        reg = regRead(POINTER_CONFIG);
        if (reg & (1 << 6)) {
#ifdef __DEBUG
            this->log_out("Error: gain not set");
#endif
            return 1;
        }
    }
    return 0;
}

uint8_t VEMLClass::setIntTime(int_time_t time) {
    regWrite(POINTER_CONFIG, time);

    /* Check user val */
    uint16_t reg = regRead(POINTER_CONFIG);
    if (!(reg & time)) {
#ifdef __DEBUG
        this->log_out("Error: gain not set");
#endif
        return 1;
    }

    return 0;
}

/**
 * @brief 16-bit write procedure
 *
 * @param reg_ptr Register pointer
 * @param data 16-bit data
 */
void regWrite(uint8_t reg_ptr, uint16_t data) {
    // /* Start transaction */
    // WIRE.beginTransmission(I2C_ADDRESS);

    // WIRE.write(reg_ptr);     // Register pointer
    // WIRE.write(data & 0xFF); // LSB data
    // WIRE.write(data >> 8);   // MSB data

    // WIRE.endTransmission(true);

    uint8_t buff[2];
    buff[0] = data & 0xFF;
    buff[1] = data >> 8;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(I2C_HANDLE, I2C_ADDRESS << 1, reg_ptr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)buff, 2, 5);
}

/**
 * @brief 16-bit read procedure
 *
 * @param reg_ptr Register pointer
 * @return uint16_t Returned data
 */
uint16_t regRead(uint8_t reg_ptr) {
    /* Variables */
    unsigned char rx_data[2] = {0};

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(I2C_HANDLE, I2C_ADDRESS << 1, reg_ptr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)rx_data, 2, 5);

    return (rx_data[1] << 8) | rx_data[0];
}
