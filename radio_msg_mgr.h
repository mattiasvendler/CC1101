/*
 * radio_msg_mgr.h
 *
 *  Created on: Mar 10, 2016
 *      Author: mattias
 */

#ifndef SRC_RADIO_RADIO_MSG_MGR_H_
#define SRC_RADIO_RADIO_MSG_MGR_H_
#include <nosys_queue.h>
#include <nosys_task.h>
#include <types.h>
#include <radio_mgr.h>
#define RADIO_MSG_SEND_QUEUE_SIZE 1
enum radio_msg_mgr_state {
	RADIO_MSG_MGR_STATE_INIT = 0,
	RADIO_MSG_MGR_STATE_RESET,
	RADIO_MSG_MGR_STATE_IDLE,
	RADIO_MSG_MGR_STATE_RX,
	RADIO_MSG_MGR_STATE_RX_ACK,
	RADIO_MSG_MSG_STATE_RX_ACK_DONE,
	RADIO_MSG_MGR_STATE_TX,
	RADIO_MSG_MGR_STATE_TX_DONE,
	RADIO_MSG_MGR_STATE_TX_ACK,
	RADIO_MSG_MGR_STATE_LBT,
	RADIO_MSG_MGR_STATE_END,
};
struct radio_msg_send_queue {
	u8_t data[61];
	u8_t len;
	u8_t resends;
	u8_t require_ack;
	void *userdata;
	void (*send_done_cb)(s32_t res, void *userdata);
	struct radio_msg_send_queue *next;
};
struct radio_msg_mgr {
	struct nosys_queue *inq;
	enum radio_msg_mgr_state state;
	void *user;
	struct radio_mgr *radio_mgr;
	s32_t (*handle_ctrl_msg_cb)(u32_t peer, u8_t msg_type, u8_t *msg, u8_t len,
			void *userdata);
	u32_t time_in_state;
	u32_t local_address;
	u8_t handle_broadcast;

};

void radio_msg_mgr_init(struct radio_msg_mgr *mgr, u32_t local_address,u8_t handle_broadcast);
void radio_msg_mgr_fn(void);
void radio_msg_mgr_data_recieved_cb(unsigned char *data, unsigned char len,
		unsigned char rssi, unsigned char lqi, void *userdata);
s32_t radio_msg_send(void *data, u8_t len, u8_t require_ack, void *userdata,
		void (*send_done_cb)(s32_t res, void *userdata));

#endif /* SRC_RADIO_RADIO_MSG_MGR_H_ */
