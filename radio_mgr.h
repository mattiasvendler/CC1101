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
#define DBG dbg_printf
#endif
#ifndef PACKET_BUF_SIZE
#define PACKET_BUF_SIZE 5
#endif
enum radio_state{
	RADIO_STATE_INIT,
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
	short rssi;
	char lqi;
};
struct radio_mgr{
	enum radio_state state;
	unsigned short time_in_state;
	void *userdata;
	void (*data_recieved)(unsigned char *data,unsigned char len,unsigned char rssi, unsigned char lqi,void *userdata);
};
void radio_state_machine(struct radio_mgr *mgr);
void radio_notify(void);
void radio_send(unsigned char *buffer,int len,void (*radio_send_done_fn)(unsigned int res,void *userdata),void *userdata);
void radio_init(void);
void radio_link_status(struct rssi_lqi *status);
void radio_fn(void);
#endif /* CC1101_RADIO_MGR_H_ */
