/*!
 *  @file Adafruit_BNO08x.cpp
 *
 *  @mainpage Adafruit BNO08x 9-DOF Orientation IMU Fusion Breakout
 *
 *  @section intro_sec Introduction
 *
 * 	I2C Driver for the Library for the BNO08x 9-DOF Orientation IMU Fusion
 * Breakout
 *
 * 	This is a library for the Adafruit BNO08x breakout:
 * 	https://www.adafruit.com/product/4754
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *  @section dependencies Dependencies
 *  This library depends on the Adafruit BusIO library
 *
 *  This library depends on the Adafruit Unified Sensor library
 *
 *  @section author Author
 *
 *  Bryan Siepert for Adafruit Industries
 *
 * 	@section license License
 *
 * 	BSD (see license.txt)
 *
 * 	@section  HISTORY
 *
 *     v1.0 - First release
 */

#include "Adafruit_BNO08x.h"

#include <stddef.h>
#include <string.h>
#include <algorithm>

#include "i2c.h"

#define I2C_MAX_BUFF_SIZE (128)

static CSTimer_t g_tim;
static sh2_SensorValue_t *_sensor_value = NULL;
static bool _reset_occurred = false;

I2C_HandleTypeDef *handle;
static int i2chal_open(sh2_Hal_t *self);
static void i2chal_close(sh2_Hal_t *self);
static int i2chal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len,
                       uint32_t *t_us);
static int i2chal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);

static uint32_t hal_getTimeUs(sh2_Hal_t *self);
static void hal_callback(void *cookie, sh2_AsyncEvent_t *pEvent);
static void sensorHandler(void *cookie, sh2_SensorEvent_t *pEvent);

/**
 * @brief Construct a new Adafruit_BNO08x::Adafruit_BNO08x object
 *
 */

/**
 * @brief Construct a new Adafruit_BNO08x::Adafruit_BNO08x object
 *
 * @param reset_pin The arduino pin # connected to the BNO Reset pin
 */
Adafruit_BNO08x::Adafruit_BNO08x(void) {}

/**
 * @brief Destroy the Adafruit_BNO08x::Adafruit_BNO08x object
 *
 */
Adafruit_BNO08x::~Adafruit_BNO08x(void)
{
    // if (temp_sensor)
    //   delete temp_sensor;
}

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  i2c_address
 *            The I2C address to be used.
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @param  sensor_id
 *            The unique ID to differentiate the sensors from others
 *    @return True if initialization was successful, otherwise false.
 */
bool Adafruit_BNO08x::begin_I2C(uint8_t i2c_address, int32_t sensor_id)
{
    _HAL.open = i2chal_open;
    _HAL.close = i2chal_close;
    _HAL.read = i2chal_read;
    _HAL.write = i2chal_write;
    _HAL.getTimeUs = hal_getTimeUs;

    return _init(sensor_id);
}

/*!  @brief Initializer for post i2c/spi init
 *   @param sensor_id Optional unique ID for the sensor set
 *   @returns True if chip identified and initialized
 */
bool Adafruit_BNO08x::_init(int32_t sensor_id)
{
    CSTimer_start(&g_tim);

    int status;

    // TODO reset

    // Open SH2 interface (also registers non-sensor event handler.)
    status = sh2_open(&_HAL, hal_callback, NULL);
    if (status != SH2_OK)
    {
        return false;
    }

    // Check connection partially by getting the product id's
    memset(&prodIds, 0, sizeof(prodIds));
    status = sh2_getProdIds(&prodIds);
    if (status != SH2_OK)
    {
        return false;
    }

    // Register sensor listener
    sh2_setSensorCallback(sensorHandler, NULL);

    return true;
}

/**
 * @brief Check if a reset has occured
 *
 * @return true: a reset has occured false: no reset has occoured
 */
bool Adafruit_BNO08x::wasReset(void)
{
    bool x = _reset_occurred;
    _reset_occurred = false;

    return x;
}

/**
 * @brief Fill the given sensor value object with a new report
 *
 * @param value Pointer to an sh2_SensorValue_t struct to fil
 * @return true: The report object was filled with a new report
 * @return false: No new report available to fill
 */
bool Adafruit_BNO08x::getSensorEvent(sh2_SensorValue_t *value)
{
    _sensor_value = value;

    value->timestamp = 0;

    if(sh2_service() != SH2_OK){
        return false;
    }

    if (value->timestamp == 0 && value->sensorId != SH2_GYRO_INTEGRATED_RV)
    {
        // no new events
        return false;
    }

    return true;
}

/**
 * @brief Enable the given report type
 *
 * @param sensorId The report ID to enable
 * @param interval_us The update interval for reports to be generated, in
 * microseconds
 * @return true: success false: failure
 */
bool Adafruit_BNO08x::enableReport(sh2_SensorId_t sensorId, uint32_t interval_us)
{
    static sh2_SensorConfig_t config;

    // These sensor options are disabled or not used in most cases
    config.changeSensitivityEnabled = false;
    config.wakeupEnabled = false;
    config.changeSensitivityRelative = false;
    config.alwaysOnEnabled = false;
    config.changeSensitivity = 0;
    config.batchInterval_us = 0;
    config.sensorSpecific = 0;

    config.reportInterval_us = interval_us;
    int status = sh2_setSensorConfig(sensorId, &config);

    if (status != SH2_OK)
    {
        return false;
    }

    return true;
}

static int i2chal_open(sh2_Hal_t *self)
{
    // Serial.println("I2C HAL open");
    uint8_t softreset_pkt[] = {5, 0, 1, 0, 1};
    bool success = false;
    for (uint8_t attempts = 0; attempts < 5; attempts++)
    {
        if (HAL_I2C_Master_Transmit(BNO08x_HANDLE, BNO08x_I2CADDR_DEFAULT<<1, softreset_pkt, 5, 100) == HAL_OK)
        {
            success = true;
            break;
        }
        HAL_Delay(30);
    }
    if (!success)
        return -1;
    HAL_Delay(300);
    return 0;
}

static void i2chal_close(sh2_Hal_t *self)
{
}

static int i2chal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len,
                       uint32_t *t_us)
{
    // Serial.println("I2C HAL read");

    // uint8_t *pBufferOrig = pBuffer;

    uint8_t header[4];
    if (HAL_I2C_Master_Receive(BNO08x_HANDLE, BNO08x_I2CADDR_DEFAULT<<1, header, 4, 100) != HAL_OK) {
        return 0;
    }

    // Determine amount to read
    uint16_t packet_size = (uint16_t)header[0] | (uint16_t)header[1] << 8;
    // Unset the "continue" bit
    packet_size &= ~0x8000;

    /*
    Serial.print("Read SHTP header. ");
    Serial.print("Packet size: ");
    Serial.print(packet_size);
    Serial.print(" & buffer size: ");
    Serial.println(len);
    */

    size_t i2c_buffer_max = I2C_MAX_BUFF_SIZE;

    if (packet_size > len) {
        // packet wouldn't fit in our buffer
        return 0;
    }
    // the number of non-header bytes to read
    uint16_t cargo_remaining = packet_size;
    uint8_t i2c_buffer[i2c_buffer_max];
    uint16_t read_size;
    uint16_t cargo_read_amount = 0;
    bool first_read = true;

    while (cargo_remaining > 0) {
        if (first_read) {
			read_size = std::min(i2c_buffer_max, (size_t)cargo_remaining);
        } else {
			read_size = std::min(i2c_buffer_max, (size_t)cargo_remaining + 4);
        }

        // Serial.print("Reading from I2C: "); Serial.println(read_size);
        // Serial.print("Remaining to read: "); Serial.println(cargo_remaining);

        if (HAL_I2C_Master_Receive(BNO08x_HANDLE, BNO08x_I2CADDR_DEFAULT<<1, i2c_buffer, read_size, 100) != HAL_OK) {
        	return 0;
        }

        if (first_read) {
			// The first time we're saving the "original" header, so include it in the
			// cargo count
			cargo_read_amount = read_size;
			memcpy(pBuffer, i2c_buffer, cargo_read_amount);
			first_read = false;
        } else {
			// this is not the first read, so copy from 4 bytes after the beginning of
			// the i2c buffer to skip the header included with every new i2c read and
			// don't include the header in the amount of cargo read
			cargo_read_amount = read_size - 4;
			memcpy(pBuffer, i2c_buffer + 4, cargo_read_amount);
        }
        // advance our pointer by the amount of cargo read
        pBuffer += cargo_read_amount;
        // mark the cargo as received
        cargo_remaining -= cargo_read_amount;
    }

    /*
    for (int i=0; i<packet_size; i++) {
        Serial.print(pBufferOrig[i], HEX);
        Serial.print(", ");
        if (i % 16 == 15) Serial.println();
    }
    Serial.println();
    */

    return packet_size;
}

static int i2chal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    size_t i2c_buffer_max = I2C_MAX_BUFF_SIZE;

    /*
    Serial.print("I2C HAL write packet size: ");
    Serial.print(len);
    Serial.print(" & max buffer size: ");
    Serial.println(i2c_buffer_max);
    */

    uint16_t write_size = std::min(i2c_buffer_max, len);
    if (HAL_I2C_Master_Transmit(BNO08x_HANDLE, BNO08x_I2CADDR_DEFAULT<<1, pBuffer, write_size, 100) != HAL_OK) {
        return 0;
    }

    return write_size;
}

static uint32_t hal_getTimeUs(sh2_Hal_t *self)
{
    uint32_t t = CSTimer_getUs(g_tim);
    
    // Serial.printf("I2C HAL get time: %d\n", t);
    return t;
}

static void hal_callback(void *cookie, sh2_AsyncEvent_t *pEvent)
{
    // If we see a reset, set a flag so that sensors will be reconfigured.
    if (pEvent->eventId == SH2_RESET)
    {
        // Serial.println("Reset!");
        _reset_occurred = true;
    }
}

// Handle sensor events.
static void sensorHandler(void *cookie, sh2_SensorEvent_t *event)
{
    int rc;

    // Serial.println("Got an event!");

    rc = sh2_decodeSensorEvent(_sensor_value, event);
    if (rc != SH2_OK)
    {
        CSLed_err();
        _sensor_value->timestamp = 0;
        return;
    }
}
