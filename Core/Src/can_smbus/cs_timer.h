/*
 * cs_timer.h
 *
 *  Created on: Oct 12, 2023
 *      Author: sen
 */

#ifndef SRC_CS_SMBUS_CS_TIMER_H_
#define SRC_CS_SMBUS_CS_TIMER_H_

#include "cs_type.h"
#include "tim.h"

#define CS_TIMER_USE_HTIM (&htim7)

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CSTimer_callback_t)(void);


typedef struct
{
    uint32_t ms;
    uint16_t us;
} CSTimer_t;

void CSTimer_init(void);

void CSTimer_bind(CSTimer_callback_t callback);

void CSTimer_start(CSTimer_t* tim);

uint32_t CSTimer_getMs(const CSTimer_t* tim);
uint32_t CSTimer_getUs(const CSTimer_t* tim);

void CSTimer_delayUs(uint32_t us);

void __CSTimer_interrupt(TIM_HandleTypeDef* htim);

#ifdef __cplusplus
}
#endif

#endif /* SRC_CS_SMBUS_CS_TIMER_H_ */
