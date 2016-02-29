/*
 * radio.c
 *
 *  Created on: Feb 19, 2016
 *      Author: mattias
 */
#include <radio_mgr.h>
#ifdef LINUX
#include <timer.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#endif
#include <cc1101.h>
struct radio_mgr radio_mgr;
#ifdef LINUX
static pthread_t t;
#endif
#ifndef NULL
#define NULL (void *)0;
#endif
static CCPACKET packet;
static CCPACKET in_packet;
static struct packet_queue *current_packet;
static unsigned int packet_flags = 0;
#define PACKET_SENT_OK 0x01
#define SENDING 0x02
#define RX_ENTERED 0x03
#define RADIO_RESET 0x04
struct cc1101_hw cc1101_hw;
extern int init_stuff;
#define RESET_TIME_IN_STATE mgr->time_in_state = 0
static const char *state[] = { "RADIO_STATE_INIT", "RADIO_STATE_RESET",
		"RADIO_STATE_IDLE", "RADIO_STATE_RX", "RADIO_STATE_TX" };
struct packet_queue {
	CCPACKET packet;
	void *userdata;
	void (*radio_send_done_fn)(unsigned int res, void *userdata);
	struct packet_queue *next;
};

static struct packet_queue *queue;

void radio_state_machine(struct radio_mgr *mgr) {
	switch (mgr->state) {
	case RADIO_STATE_INIT: {

		int i;
		for (i = 0; i < 20; i++) {
			packet.data[i] = i + 1;
		}
		packet.length = 20;
		queue = NULL;
		packet_flags &= ~RADIO_RESET;
		RESET_TIME_IN_STATE;
		init_stuff = 1;
//		setIdleState();
		mgr->state = RADIO_STATE_IDLE;
		DBG("%s\n", state[mgr->state]);
		break;
	}
	case RADIO_STATE_RESET:
		if (!(packet_flags & RADIO_RESET)) {
			packet_flags |= RADIO_RESET;
			DBG("%s\n", state[mgr->state]);
			DBG("RESET radio1\n");
			init_stuff = 0;
			CC1101_init(&cc1101_hw);
			DBG("RESET radio2\n");
			RESET_TIME_IN_STATE;

			mgr->state = RADIO_STATE_INIT;
		}
		break;
	case RADIO_STATE_IDLE:
		if (!(packet_flags & RX_ENTERED)) {
//			DBG("%s\n", state[mgr->state]);
			if (CC1101_rx_mode()) {
				DBG("RX entered\n");
				packet_flags |= RX_ENTERED;
			} else if (mgr->time_in_state > 1000) {
				RESET_TIME_IN_STATE;
				mgr->state = RADIO_STATE_RESET;
			}
			packet_flags &= (SENDING | PACKET_SENT_OK);
		}
		if (queue) {
//				DBG("RADIO_STATE_IDLE\n");
			packet_flags &= ~RX_ENTERED;
			RESET_TIME_IN_STATE;
			mgr->state = RADIO_STATE_TX;
		}
		break;
	case RADIO_STATE_RX:
//		DBG("%s\n", state[mgr->state]);
		if (CC1101_receiveData(&in_packet)) {
			DBG("Len:%d\nLQI %d\n RSSI %d SEQUENCE %d\n", in_packet.length,
					in_packet.lqi, in_packet.rssi, in_packet.data[0]);
		radio_send(in_packet.data,in_packet.length,NULL,NULL);
	}
	RESET_TIME_IN_STATE;
	packet_flags &= ~RX_ENTERED;
	setIdleState();
	radio_mgr.state = RADIO_STATE_IDLE;
	break;
case RADIO_STATE_TX: {
	if (!(packet_flags & SENDING)) {
		DBG("%s\n", state[mgr->state]);
		DBG("SEND START\n");
		packet_flags &= !RX_ENTERED;
		current_packet = queue;
		if (!CC1101_sendData(current_packet->packet)) {
			RESET_TIME_IN_STATE;
			DBG("SEND FAILED1\n");
			radio_mgr.state = RADIO_STATE_IDLE;
		} else {
			DBG("WAIT FOR INTERRUPT\n");
			packet_flags |= SENDING;
			RESET_TIME_IN_STATE;
		}
		queue = current_packet->next;
	} else if (packet_flags & PACKET_SENT_OK) {
		boolean tx_fifo_empty = CC1101_tx_fifo_empty();
		if (!tx_fifo_empty) {
			DBG("SEND FAILED2 size %d %d\n",(readStatusReg(CC1101_TXBYTES) & 0x7F),current_packet->packet.length);
		}else{
			DBG("SEND OKsize %d %d\n",(readStatusReg(CC1101_TXBYTES) & 0x7F),current_packet->packet.length);
		}
		if (current_packet->radio_send_done_fn) {
			current_packet->radio_send_done_fn(tx_fifo_empty,
					current_packet->userdata);
		}
		RESET_TIME_IN_STATE;
		setIdleState();
		mgr->state = RADIO_STATE_IDLE;
		free(current_packet);
		packet_flags &= !SENDING;
	} else if ((packet_flags & SENDING) && mgr->time_in_state > 10) {
		setIdleState();
		mgr->state = RADIO_STATE_IDLE;
		DBG("TX TIMEOUT\n");
		mgr->state = RADIO_STATE_IDLE;
	}

	break;
}
	}

}

#ifdef LINUX
static void *radio_mgr_thread(void *arg) {
	struct radio_mgr *mgr = (struct radio_mgr *) arg;
	while (1) {

		radio_state_machine(mgr);
		const struct timespec ts = {0, 1000000};
		struct timespec rem;
		sched_yield();
		nanosleep(&ts, &rem);
	}
	return arg;
}
#endif
void radio_send(unsigned char *buffer, int len,
		void (*radio_send_done_fn)(unsigned int res, void *userdata),
		void *userdata) {
	DBG("Paket len %d\n",len);
//	int i;
//	for(i=0;i<len;i++){
//		DBG("%02X",buffer[i]);
//	}
//	DBG("\n");
	struct packet_queue *pq = malloc(sizeof(struct packet_queue));
	memset(pq, 0, sizeof(struct packet_queue));

	memcpy(&pq->packet.data[0], buffer, len);
	pq->packet.length = len;
	pq->userdata = userdata;
	pq->radio_send_done_fn = radio_send_done_fn;
	pq->next = NULL
	;
	if (queue) {
		queue->next = pq;
		queue = pq;
	} else {
		queue = pq;
	}
}
void radio_notify() {
	if (radio_mgr.state == RADIO_STATE_TX && (packet_flags & SENDING)) {
		packet_flags |= PACKET_SENT_OK;
	} else if (radio_mgr.state == RADIO_STATE_IDLE) {
		radio_mgr.state = RADIO_STATE_RX;
	} else {
		DBG("MISSED \n");
	}
}
static void radio_mgr_timer_cb(void) {
	radio_fn();
}
void radio_init() {
	radio_mgr.state = RADIO_STATE_IDLE;

#ifdef LINUX
	timer_add_callback(radio_mgr_timer_cb);
	int res = pthread_create(&t, NULL, radio_mgr_thread, (void *) &radio_mgr);

	if (res != 0) {
		perror(NULL);
	}
#endif
}
void radio_fn() {

}
