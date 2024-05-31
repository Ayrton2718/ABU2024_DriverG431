/*
 * cs_panel_type.h
 *
 *  Created on: Apr 12, 2024
 *      Author: sen
 */

#ifndef SRC_CS_PANEL_TYPE_H_
#define SRC_CS_PANEL_TYPE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    uint8_t red_zone : 1;
    uint8_t start_or_retry : 1;
    uint8_t power_24v : 1;
    uint8_t start : 1;
    uint8_t boot : 1;
    uint8_t kill : 1;
    uint8_t strategy : 1;
    uint8_t reserved : 1;
    uint8_t checksum : 8;
}__attribute__((__packed__)) CSPanel_s2m_t;

typedef struct{
    uint8_t running       	: 1;
    uint8_t booting      	: 1;
    uint8_t boot_err 		: 1;
    uint8_t is_red_zone     : 1;
    uint8_t is_blue_zone    : 1;
    uint8_t retry   		: 1;
    uint8_t strategy 		: 1;
    uint8_t io_err          : 1;
    uint8_t checksum 		: 8;
}__attribute__((__packed__)) CSPanel_m2s_t;

#ifdef __cplusplus
}
#endif


#endif /* SRC_CS_PANEL_TYPE_H_ */
