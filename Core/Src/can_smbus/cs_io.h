/*
 * cs_io.h
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#ifndef SRC_CAN_SMBUS_CS_IO_H_
#define SRC_CAN_SMBUS_CS_IO_H_

#include "cs_type.h"
#include "fdcan.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CSIO_HCAN ((&hfdcan1))

typedef CSType_bool_t (*CSIo_callback_t)(CSReg_t reg, const uint8_t* data, size_t len);

void CSIo_init(void);

void CSIo_bind(CSType_appid_t appid, CSIo_callback_t callback);

void CSIo_sendUser(CSReg_t reg, const uint8_t* data, uint8_t len);

CSType_bool_t CSIo_isSafetyOn(void);

void CSIo_process(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_CAN_SMBUS_CS_IO_H_ */
