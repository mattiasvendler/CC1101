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
enum radio_state{
	RADIO_STATE_INIT,
	RADIO_STATE_RESET,
	RADIO_STATE_IDLE,
	RADIO_STATE_RX,
	RADIO_STATE_TX
};
struct radio_mgr{
	enum radio_state state;
	unsigned short time_in_state;
};
void radio_state_machine(struct radio_mgr *mgr);
void radio_notify();
void radio_send(unsigned char *buffer,int len,void (*radio_send_done_fn)(unsigned int res,void *userdata),void *userdata);
void radio_init();
void radio_fn();
#endif /* CC1101_RADIO_MGR_H_ */
