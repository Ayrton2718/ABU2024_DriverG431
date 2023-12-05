// /*
//  * cs_io.c
//  *
//  *  Created on: Oct 27, 2023
//  *      Author: sen
//  */


// #include "cs_io.h"

// #include "cs_id.h"
// #include "cs_led.h"

// #define CS_IO_SEND_BUFFER        (8)

// typedef struct
// {
//     uint8_t tx_data[8];
//     CAN_TxHeaderTypeDef header;
// } CSIo_packet_t;

// uint8_t         g_send_wp;
// uint8_t         g_send_rp;
// uint8_t         g_send_count;
// CSIo_packet_t   g_send_buffer[CS_IO_SEND_BUFFER];

// static CAN_HandleTypeDef* g_hcan;
// static CSType_id_t   g_my_id;
// static CSType_appid_t g_appid;
// static CSIo_callback_t g_callback;

// static uint32_t g_safety_time;


// static void CSIo_setFilterMask(uint32_t fifo_id, uint16_t id1, uint16_t mask1, uint16_t id2, uint16_t mask2);
// static CSType_bool_t CSIo_dummyCallback(CSReg_t reg, const uint8_t* data, size_t len){return CSTYPE_FALSE;}
// static void CSIo_send(CSType_reg_t reg, const uint8_t* data, uint8_t len);


// void CSIo_init(CAN_HandleTypeDef* hcan)
// {
//     g_hcan = hcan;
//     g_my_id = (CSId_getId() & 0b01111);

//     CSIo_setFilterMask(CAN_FILTER_FIFO0, g_my_id << 6,  0b11111 << 6, 0x7FF, 0x7FF); // m2s registers
//     CSIo_setFilterMask(CAN_FILTER_FIFO1, 0x00000 << 6,  0b11111 << 6, 0x7FF, 0x7FF); // broad cast

//     HAL_CAN_Start(g_hcan);
// 	HAL_CAN_ActivateNotification(g_hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
// 	HAL_CAN_ActivateNotification(g_hcan, CAN_IT_RX_FIFO1_MSG_PENDING);

//     g_appid = CSType_appid_UNKNOWN;
//     g_callback = CSIo_dummyCallback;

//     g_safety_time = 0;

//     g_send_wp = 0;
//     g_send_rp = 0;
//     g_send_count = 0;
//     memset(g_send_buffer, 0x00, sizeof(g_send_buffer));
// }


// void CSIo_bind(CSType_appid_t appid, CSIo_callback_t callback)
// {
//     g_appid = appid;
//     g_callback = callback;
// }

// void CSIo_sendUser(CSType_reg_t reg, const uint8_t* data, uint8_t len)
// {
//     CSIo_send(CSTYPE_MAKE_WRITE_REG(CSTYPE_MAKE_USER_REG(reg)), data, len);
// }


// static void CSIo_send(CSType_reg_t reg, const uint8_t* data, uint8_t len)
// {
//     CSIo_packet_t packet;
//     uint32_t TxMailbox;

//     packet.header.RTR = CAN_RTR_DATA;                           // フレームタイプはデータフレーム
//     packet.header.IDE = CAN_ID_STD;                             // 標準ID(11ﾋﾞｯﾄ)
//     packet.header.StdId = CSTYPE_MAKE_S2M_CAN_ID(g_my_id, reg); // CAN ID
//     packet.header.DLC = len;
//     packet.header.TransmitGlobalTime = DISABLE;  // ???
//     memcpy(packet.tx_data, data, sizeof(uint8_t) * len);

//     if(HAL_CAN_GetTxMailboxesFreeLevel(g_hcan) != 0)
//     {
// 		if(HAL_CAN_AddTxMessage(g_hcan, &packet.header, packet.tx_data, &TxMailbox) == HAL_OK)
// 		{
//             CSLed_tx();
//         }else{
//             CSLed_err();
//         }
//     }else{
// 		if(g_send_count == CS_IO_SEND_BUFFER)
// 		{
// 			g_send_rp++;
// 			g_send_count--;
//             CSLed_err();
// 		}
// 		g_send_buffer[g_send_wp % CS_IO_SEND_BUFFER] = packet;
// 		g_send_wp++;
// 		g_send_count++;
//     }
// }


// CSType_bool_t CSIo_isSafetyOn(void)
// {
//     return (g_safety_time < HAL_GetTick());
// }

// void CSIo_process(void)
// {
//     size_t send_count = g_send_count;
//     for(size_t i = 0; (i < send_count) && (HAL_CAN_GetTxMailboxesFreeLevel(g_hcan) != 0); i++)
//     {
//         uint32_t TxMailbox;
//         CSIo_packet_t packet = g_send_buffer[g_send_rp & CS_IO_SEND_BUFFER];
//         if(HAL_CAN_AddTxMessage(g_hcan, &packet.header, packet.tx_data, &TxMailbox) != HAL_OK)
//         {
//             CSLed_tx();
//         }else{
//             CSLed_err();
//         }
//         g_send_rp++;
//         g_send_count--;
//     }
// }

// // Register
// void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
// {
// 	CAN_RxHeaderTypeDef RxHeader;
// 	uint8_t data[8];

// 	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, data) == HAL_OK)
// 	{
//         uint32_t can_id = RxHeader.StdId;
//         uint8_t len = RxHeader.DLC;

//         if(CSTYPE_IS_SYS_REG(can_id))
//         {
//             CSType_reg_t reg = CSTYPE_GET_SYS_REG(can_id);
//             if(CSTYPE_IS_ACK_REG(can_id))
//             {
//                 CSType_ack_t ack;
//                 ack.checksum = 0;
//                 for(size_t i = 0; i < len; i++)
//                 {
//                     ack.checksum += data[i];
//                 }
//                 CSIo_send(CSTYPE_MAKE_ACK_REG(CSTYPE_MAKE_SYS_REG(reg)), (const uint8_t*)&ack, sizeof(CSType_ack_t));
//             }

//             switch(reg)
//             {
//             case CSReg_m2sSystem_REQ_APPID:{
//                 uint8_t appid = (uint8_t)g_appid;
//                 CSIo_send(CSTYPE_MAKE_WRITE_REG(CSTYPE_MAKE_SYS_REG(CSReg_s2mSystem_APPID)), &appid, sizeof(uint8_t));
//                 CSLed_rx();
//                 }break;

//             default:
//                 break;
//             }
//         }
//         else
//         {
//             CSType_reg_t reg = CSTYPE_GET_USER_REG(can_id);
//             if(CSTYPE_IS_ACK_REG(can_id))
//             {
//                 CSType_ack_t ack;
//                 ack.checksum = 0;
//                 for(size_t i = 0; i < len; i++)
//                 {
//                     ack.checksum += data[i];
//                 }
//                 CSIo_send(CSTYPE_MAKE_ACK_REG(CSTYPE_MAKE_USER_REG(reg)), (const uint8_t*)&ack, sizeof(CSType_ack_t));
//             }

//             if(g_callback(reg, data, len))
//             {
//                 CSLed_rx();
//             }
//         }		
// 	}else{
//         CSLed_err();
// 	}
// }


// // BRC
// void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
// {
// 	CAN_RxHeaderTypeDef RxHeader;
// 	uint8_t data[8];

// 	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, data) == HAL_OK)
// 	{
// 		uint32_t can_id = RxHeader.StdId;
//         uint8_t len = RxHeader.DLC;

//         if(CSTYPE_IS_BRC_PACKET(can_id))
//         {
//             CSType_reg_t reg = CSTYPE_GET_BRC_REG(can_id);
//             switch (reg)
//             {
//             case CSType_brcReg_Safety:
//                 g_safety_time = HAL_GetTick() + 500;
//                 CSLed_rx();
//                 break;
            
//             case CSType_brcReg_Unsafe:
//                 if(len == 4)
//                 {
//                     if((data[3] == 'U') && (data[2] == 'N') && (data[1] == 'S') && (data[0] == 'F'))
//                     {
//                         g_safety_time = 0;
//                         CSLed_rx();
//                     }
//                 }
//                 break;

//             case CSType_brcReg_ChipInit:
//                 break;

//             default:
//                 break;
//             }
//         }
// 	}else{
//         CSLed_err();
// 	}
// }


// static void CSIo_setFilterMask(uint32_t fifo_id, uint16_t id1, uint16_t mask1, uint16_t id2, uint16_t mask2)
// {
//     id1   =  id1 << 5;          // フィルターID1
// 	mask1 = (mask1 << 5) | 0x8; // フィルターマスク1
// 	id2   =  id2 << 5;          // フィルターID2
// 	mask2 = (mask2 << 5) | 0x8; // フィルターマスク2

// 	CAN_FilterTypeDef filter;
// 	filter.FilterIdHigh         = id1;                        // フィルターID(上位16ビット)
// 	filter.FilterIdLow          = id2;                        // フィルターID(下位16ビット)
// 	filter.FilterMaskIdHigh     = mask1;                        // フィルターマスク(上位16ビット)
// 	filter.FilterMaskIdLow      = mask2;                        // フィルターマスク(下位16ビット)
// 	filter.FilterScale          = CAN_FILTERSCALE_16BIT;    // フィルタースケール
// 	filter.FilterFIFOAssignment = fifo_id;         			// フィルターに割り当てるFIFO
// 	filter.FilterBank           = 13 * fifo_id;                        // フィルターバンクNo
// 	filter.FilterMode           = CAN_FILTERMODE_IDMASK;    // list mode
// 	filter.SlaveStartFilterBank = 14;                       // スレーブCANの開始フィルターバンクNo
// 	filter.FilterActivation     = CAN_FILTER_ENABLE;          // フィルター無効／有効

// 	if (HAL_CAN_ConfigFilter(g_hcan, &filter) != HAL_OK)
// 	{
// 		CSLed_err();
// 	}

// }