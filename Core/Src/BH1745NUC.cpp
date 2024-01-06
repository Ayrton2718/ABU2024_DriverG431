/*****************************************************************************
  BH1745NUC.cpp

 Copyright (c) 2016 ROHM Co.,Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
******************************************************************************/
#include "BH1745NUC.h"

#include <i2c.h>
#include <string.h>

#define I2C_HANDLE ((&hi2c2))
//#define __DEBUG

BH1745NUC::BH1745NUC(int slave_address)
{
  _device_address = slave_address;
}

byte BH1745NUC::init(void)
{
  byte rc;
  unsigned char reg;

  rc = read(BH1745NUC_SYSTEM_CONTROL, &reg, sizeof(reg));
  if (rc != 0) {
#ifdef __DEBUG
    this->log_out("Can't access BH1745NUC");
#endif /*DEBUG*/
    return (rc);
  }
  reg = reg & 0x3F;
#ifdef __DEBUG
  this->log_out("BH1745NUC Part ID Value = ");
  this->log_out(std::to_string(reg));
#endif /*DEBUG*/

  if (reg != BH1745NUC_PART_ID_VAL) {
#ifdef __DEBUG
    this->log_out("Can't find BH1745NUC");
#endif /*DEBUG*/
    return (rc);
  }

  rc = read(BH1745NUC_MANUFACTURER_ID, &reg, sizeof(reg));
  if (rc != 0) {
#ifdef __DEBUG
    this->log_out("Can't access BH1745NUC");
#endif /*DEBUG*/
    return (rc);
  }

#ifdef __DEBUG
  this->log_out("BH1745NUC MANUFACTURER ID Register Value = ");
  this->log_out(std::to_string(reg));
#endif /*DEBUG*/

  if (reg != BH1745NUC_MANUFACT_ID_VAL) {
#ifdef __DEBUG
    this->log_out("Can't find BH1745NUC");
#endif /*DEBUG*/
    return (rc);
  }

  reg = BH1745NUC_MODE_CONTROL1_VAL;
  rc = write(BH1745NUC_MODE_CONTROL1, &reg, sizeof(reg));
  if (rc != 0) {
#ifdef __DEBUG
    this->log_out("Can't write BH1745NUC MODE_CONTROL1 register");
#endif /*DEBUG*/
    return (rc);
  }

  reg = BH1745NUC_MODE_CONTROL2_VAL;
  rc = write(BH1745NUC_MODE_CONTROL2, &reg, sizeof(reg));
  if (rc != 0) {
#ifdef __DEBUG
    this->log_out("Can't write BH1745NUC MODE_CONTROL2 register");
#endif /*DEBUG*/
    return (rc);
  }

  reg = BH1745NUC_MODE_CONTROL3_VAL;
  rc = write(BH1745NUC_MODE_CONTROL3, &reg, sizeof(reg));
  if (rc != 0) {
#ifdef __DEBUG
    this->log_out("Can't write BH1745NUC MODE_CONTROL3 register");
#endif /*DEBUG*/
    return (rc);
  }

  return 0;
}

HAL_StatusTypeDef BH1745NUC::get_rawval(unsigned char *data)
{
  HAL_StatusTypeDef rc;

  rc = read(BH1745NUC_RED_DATA_LSB, data, 8);
  if (rc != 0) {
#ifdef __DEBUG
    this->log_out("Can't get BH1745NUC RGBC value");
#endif /*DEBUG*/
  }

  return (rc);
}

HAL_StatusTypeDef BH1745NUC::get_val(unsigned short *data)
{
  HAL_StatusTypeDef rc;
  unsigned char val[8];

  memset(val, 0x00, sizeof(val));

  rc = get_rawval(val);
  if (rc != 0) {
    return (rc);
  }

  data[0] = ((unsigned short)val[1] << 8) | val[0];
  data[1] = ((unsigned short)val[3] << 8) | val[2];
  data[2] = ((unsigned short)val[5] << 8) | val[4];
  data[3] = ((unsigned short)val[7] << 8) | val[6];

  return (rc);
}

HAL_StatusTypeDef BH1745NUC::write(unsigned char memory_address, unsigned char *data, unsigned char size)
{
  uint8_t buff[64];
  buff[0] = memory_address;
  memcpy(&buff[1], data, size);

  // HAL_I2C_Master_Transmit(&hi2c1, _device_address, buff, size + 1, 100);
  return HAL_I2C_Mem_Write(I2C_HANDLE, _device_address << 1, memory_address, I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, size, 5);
}

HAL_StatusTypeDef BH1745NUC::read(unsigned char memory_address, unsigned char *data, int size)
{
  // byte rc;
  // unsigned char cnt;
  // uint8_t buff[64];

  // if(HAL_I2C_Master_Transmit(I2C_HANDLE, _device_address, &memory_address, 1, 100) == HAL_OK)
  // {
  //   return HAL_I2C_Master_Receive(I2C_HANDLE, _device_address, data, size, 100);
  // }

  // return HAL_ERROR;
  return HAL_I2C_Mem_Read(I2C_HANDLE, _device_address << 1, memory_address, I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, size, 5);
}

void BH1745NUC::clear_err(void)
{
    __HAL_I2C_DISABLE(I2C_HANDLE);
    
    I2C_HANDLE->ErrorCode = HAL_I2C_ERROR_NONE;
    I2C_HANDLE->State = HAL_I2C_STATE_READY;
    // I2C_HANDLE->PreviousState = I2C_STATE_NONE;
    I2C_HANDLE->Mode = HAL_I2C_MODE_NONE;

      __HAL_I2C_ENABLE(I2C_HANDLE);
}
