/*
 * cs_id.h
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#ifndef SRC_CAN_SMBUS_CS_ID_H_
#define SRC_CAN_SMBUS_CS_ID_H_

#include "cs_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CS_ID_FIXED_ADDR (CSId_8)

void CSId_init(void);

CSId_t CSId_getId(void);

void CSId_process(CSType_bool_t is_safety_on);

#ifdef __cplusplus
}
#endif


#endif /* SRC_CAN_SMBUS_CS_ID_H_ */
