/*
 * cs_led.h
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#ifndef SRC_CAN_SMBUS_CS_LED_H_
#define SRC_CAN_SMBUS_CS_LED_H_

#include "cs_type.h"


#ifdef __cplusplus
extern "C" {
#endif

void CSLed_init(void);

// tx : toggle
void CSLed_tx(void);

// rx : toggle
void CSLed_rx(void);

// Hang up
// tx : on, rx : on, er : on
void CSLed_hungUp(void);

// Other Error
// tx : blink, rx : blink, er : on
void CSLed_err(void);

// Control function
void CSLed_process(CSType_bool_t is_safety_on);

#ifdef __cplusplus
}
#endif



#endif /* SRC_CAN_SMBUS_CS_LED_H_ */
