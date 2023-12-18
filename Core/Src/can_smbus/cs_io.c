/*
 * cs_io.c
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */


#include "cs_io.h"

#include "cs_id.h"
#include "cs_led.h"

#define CS_IO_SEND_BUFFER        (8)

typedef struct
{
    uint8_t tx_data[8];
    FDCAN_TxHeaderTypeDef header;
} CSIo_packet_t;

uint8_t         g_send_wp;
uint8_t         g_send_rp;
uint8_t         g_send_count;
CSIo_packet_t   g_send_buffer[CS_IO_SEND_BUFFER];

static uint16_t   g_my_id;
static CSType_appid_t g_appid;
static CSIo_canCallback_t g_can_callback;
static CSIo_resetCallback_t g_reset_callback;

static uint32_t g_safety_time;


static void CSIo_setFilterMask(uint32_t index, uint32_t fifo_id, uint16_t id1, uint16_t mask1);
static CSType_bool_t CSIo_dummyCallback(CSReg_t reg, const uint8_t* data, size_t len){return CSTYPE_FALSE;}
static void CSIo_dummyResetCallback(void){}
static void CSIo_send(uint16_t reg, const uint8_t* data, uint8_t len);


void CSIo_init(void)
{
    g_my_id = (CSId_getId() & 0b01111);

    CSIo_setFilterMask(0, FDCAN_FILTER_TO_RXFIFO0, g_my_id << 6,   0b01111 << 6); // m2s registers
    CSIo_setFilterMask(1, FDCAN_FILTER_TO_RXFIFO1, 0b00000 << 6,   0b11111 << 6); // broad cast

    if(HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_FDCAN_Start(CSIO_HCAN) != HAL_OK){ 
        Error_Handler();
    }
    
    if (HAL_FDCAN_ActivateNotification(CSIO_HCAN, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_FDCAN_ActivateNotification(CSIO_HCAN, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0) != HAL_OK)
    {
        Error_Handler();
    }

    g_appid = CSType_appid_UNKNOWN;
    g_can_callback = CSIo_dummyCallback;
    g_reset_callback = CSIo_dummyResetCallback;

    g_safety_time = 0;

    g_send_wp = 0;
    g_send_rp = 0;
    g_send_count = 0;
    memset(g_send_buffer, 0x00, sizeof(g_send_buffer));
}


void CSIo_bind(CSType_appid_t appid, CSIo_canCallback_t callback, CSIo_resetCallback_t reset_callback)
{
    g_appid = appid;
    g_can_callback = callback;
    g_reset_callback = reset_callback;
}

void CSIo_sendUser(CSReg_t reg, const uint8_t* data, uint8_t len)
{
    CSIo_send(CSTYPE_MAKE_WRITE_REG(CSTYPE_MAKE_USER_REG(reg)), data, len);
}


static void CSIo_send(uint16_t reg, const uint8_t* data, uint8_t len)
{
    CSIo_packet_t packet;

    packet.header.TxFrameType = FDCAN_DATA_FRAME;
    packet.header.IdType = FDCAN_STANDARD_ID;
    packet.header.Identifier = CSTYPE_MAKE_S2M_CAN_ID(g_my_id, reg);
    packet.header.DataLength = len << 16;
    packet.header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    packet.header.BitRateSwitch = FDCAN_BRS_OFF;
    packet.header.FDFormat = FDCAN_CLASSIC_CAN;
    packet.header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    packet.header.MessageMarker = 0;
    memcpy(packet.tx_data, data, sizeof(uint8_t) * len);

    if(HAL_FDCAN_GetTxFifoFreeLevel(CSIO_HCAN) != 0)
    {
        if (HAL_FDCAN_AddMessageToTxFifoQ(CSIO_HCAN, &packet.header, packet.tx_data) == HAL_OK)
        {
            CSLed_tx();
        }else{
            CSLed_err();
        }
    }else{
		if(g_send_count == CS_IO_SEND_BUFFER)
		{
			g_send_rp++;
			g_send_count--;
            CSLed_err();
		}
		g_send_buffer[g_send_wp % CS_IO_SEND_BUFFER] = packet;
		g_send_wp++;
		g_send_count++;
    }
}


CSType_bool_t CSIo_isSafetyOn(void)
{
    return (g_safety_time < HAL_GetTick());
}

void CSIo_process(void)
{
    size_t send_count = g_send_count;
    for(size_t i = 0; (i < send_count) && (HAL_FDCAN_GetTxFifoFreeLevel(CSIO_HCAN) != 0); i++)
    {
        CSIo_packet_t packet = g_send_buffer[g_send_rp & CS_IO_SEND_BUFFER];
        if(HAL_FDCAN_AddMessageToTxFifoQ(CSIO_HCAN, &packet.header, packet.tx_data) == HAL_OK)
        {
            CSLed_tx();
        }else{
            CSLed_err();
        }
        g_send_rp++;
        g_send_count--;
    }
}

// Register
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
    {
        FDCAN_RxHeaderTypeDef   RxHeader;
        uint8_t data[8];

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, data) == HAL_OK)
        {
            uint32_t can_id = RxHeader.Identifier;
            uint8_t len = RxHeader.DataLength >> 16;

            if(CSTYPE_IS_M2S_PACKET(can_id))
            {
                if(CSTYPE_IS_SYS_REG(CSTYPE_GET_PACKET_REG(can_id)))
				{
					CSReg_t reg = CSTYPE_GET_SYS_REG(CSTYPE_GET_PACKET_REG(can_id));
					if(CSTYPE_IS_ACK_REG(CSTYPE_GET_PACKET_REG(can_id)))
					{
						CSType_ack_t ack;
						ack.checksum = 0;
						for(size_t i = 0; i < len; i++)
						{
							ack.checksum += data[i];
						}
						CSIo_send(CSTYPE_MAKE_ACK_REG(CSTYPE_MAKE_SYS_REG(reg)), (const uint8_t*)&ack, sizeof(CSType_ack_t));
					}

					switch(reg)
					{
					case CSReg_m2sSystem_REQ_APPID:{
						uint8_t appid = (uint8_t)g_appid;
						CSIo_send(CSTYPE_MAKE_WRITE_REG(CSTYPE_MAKE_SYS_REG(CSReg_s2mSystem_APPID)), &appid, sizeof(uint8_t));
						CSLed_rx();
						}break;

					default:
						break;
					}
				}
				else
				{
					CSReg_t reg = CSTYPE_GET_USER_REG(CSTYPE_GET_PACKET_REG(can_id));
					if(CSTYPE_IS_ACK_REG(CSTYPE_GET_PACKET_REG(can_id)))
					{
						CSType_ack_t ack;
						ack.checksum = 0;
						for(size_t i = 0; i < len; i++)
						{
							ack.checksum += data[i];
						}
						CSIo_send(CSTYPE_MAKE_ACK_REG(CSTYPE_MAKE_USER_REG(reg)), (const uint8_t*)&ack, sizeof(CSType_ack_t));
					}

					if(g_can_callback(reg, data, len))
					{
						CSLed_rx();
					}else{
                        CSLed_err();
					}
				}
            }
            else
            {
                CSLed_err();
            }
        }else{
            CSLed_err();
        }
    }
}


// BRC
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
    if((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != 0)
    {
        FDCAN_RxHeaderTypeDef   RxHeader;
        uint8_t data[8];

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &RxHeader, data) == HAL_OK)
        {
            uint32_t can_id = RxHeader.Identifier;
            uint8_t len = RxHeader.DataLength >> 16;

            CSType_brcReg_t reg = CSTYPE_GET_BRC_REG(CSTYPE_GET_PACKET_REG(can_id));
			switch (reg)
			{
			case CSType_brcReg_Safety:
				g_safety_time = HAL_GetTick() + CSTYPE_SAFETY_TIMEOUT;
				CSLed_rx();
				break;

			case CSType_brcReg_Unsafe:
				if(len == 4)
				{
					if((data[3] == 'U') && (data[2] == 'N') && (data[1] == 'S') && (data[0] == 'F'))
					{
						g_safety_time = 0;
						CSLed_rx();
					}
				}
				break;

			case CSType_brcReg_Reset:
                if(len == 4)
				{
					if((data[3] == 'R') && (data[2] == 'E') && (data[1] == 'S') && (data[0] == 'T'))
					{
                        g_reset_callback();
						CSLed_rx();
					}
				}
				break;

			default:
				break;
			}
        }
        else
        {
            CSLed_err();
        }
    }
}


static void CSIo_setFilterMask(uint32_t index, uint32_t fifo_id, uint16_t id1, uint16_t mask1)
{
	FDCAN_FilterTypeDef filter;
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterType = FDCAN_FILTER_MASK;
    filter.FilterIndex = index;
    filter.FilterConfig = fifo_id;
    filter.FilterID1 = id1;
    filter.FilterID2 = mask1;

    if (HAL_FDCAN_ConfigFilter(CSIO_HCAN, &filter) != HAL_OK){
        Error_Handler();
    }
}
