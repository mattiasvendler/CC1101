/*
 * radio_mgr.h
 *
 *  Created on: Feb 19, 2016
 *      Author: mattias
 */

#ifndef CC1101_RADIO_MGR_H_
#define CC1101_RADIO_MGR_H_

#include <cc1101.h>
#ifdef LINUX
#define DBG printf
#else
#include <debug.h>
#include <nosys_queue.h>
#include <nosys_timer.h>
#include <types.h>
#define DBG dbg_printf
#endif
#ifndef PACKET_BUF_SIZE
#define PACKET_BUF_SIZE 1
#endif
enum radio_state{
	RADIO_STATE_INIT = 0,
	RADIO_STATE_RESET,
	RADIO_STATE_IDLE,
	RADIO_STATE_RX,
	RADIO_STATE_TX

};

struct packet_queue {
	CCPACKET packet;
	void *userdata;
	void (*radio_send_done_fn)(unsigned int res, void *userdata);
	struct packet_queue *next;
};

struct rssi_lqi{
	s16_t rssi;
	char lqi;
};
struct radio_mgr{
	struct nosys_queue *inq;
	struct nosys_timer *timer;
	enum radio_state state;
	u32_t time_in_state;
	void *userdata;
	struct nosys_queue *reset_queue;
	void (*radio_mgr_data_recieved_fn)(u8_t *data,u8_t len,void *user_data);
	void (*data_recieved)(unsigned char *data,unsigned char len,unsigned char rssi, unsigned char lqi,void *userdata);
	void (*enable_interrupt)(void);
	void (*disable_interrupt)(void);
};
enum radio_state radio_state_machine(struct radio_mgr *mgr, struct nosys_msg *msg);
void radio_notify(struct radio_mgr *mgr);
s32_t radio_send(unsigned char *buffer,int len,void (*radio_send_done_fn)(unsigned int res,void *userdata),void *userdata);
void radio_init(void);
void radio_link_status(struct rssi_lqi *status);
void radio_fn(void);
void radio_mgr_reset(void *mgr, void *reply_queue);
#endif /* CC1101_RADIO_MGR_H_ */
