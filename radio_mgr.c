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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#endif
#include <string.h>
#include <cc1101.h>
#include <nosys_task.h>
#include <nosys_queue.h>
#include <error_codes.h>
struct radio_mgr radio_mgr;
#ifdef LINUX
static pthread_t t;
#endif
#ifndef NULL
#define NULL (void *)0;
#endif
static CCPACKET in_packet;
static struct packet_queue *current_packet;
static struct packet_queue packet_buf;
//static unsigned int packet_flags = 0;
#define PACKET_SENT_OK 0x01
#define SENDING 0x02
#define RX_ENTERED 0x03
#define RADIO_RESET 0x04
struct cc1101_hw cc1101_hw;
#define RESET_TIME_IN_STATE mgr->time_in_state = 0
static byte last_marcstate = 0;
//static struct packet_queue packet_buff[PACKET_BUF_SIZE];
//static unsigned char packet_used[PACKET_BUF_SIZE];
static struct packet_queue *queue;
extern struct rssi_lqi status;
#ifdef STATE_DEBUG
static const char *state[] = { "RADIO_STATE_INIT", "RADIO_STATE_RESET",
		"RADIO_STATE_IDLE", "RADIO_STATE_RX", "RADIO_STATE_TX" };
#endif
//static struct packet_queue * alloc_packet(void) {
//	struct packet_queue *q = NULL;
//	unsigned char i;
//	for (i = 0; i < PACKET_BUF_SIZE; i++) {
//		if (packet_used[i] == 0) {
//			packet_used[i] = 1;
//			q = &packet_buff[i];
//			return q;
//		}
//	}
//	return q;
//}

//static void packet_free(struct packet_queue *q) {
//	unsigned char i;
//	for (i = 0; i < PACKET_BUF_SIZE; i++) {
//		if (&packet_buff[i] == q) {
//			packet_used[i] = 0;
//			memset(&packet_buff[i], 0, sizeof(struct packet_queue));
//		}
//	}
//}

enum radio_state radio_state_machine(struct radio_mgr *mgr,
		struct nosys_msg *msg) {
	enum radio_state next_state = mgr->state;
	switch (mgr->state) {
	case RADIO_STATE_INIT: {

//		queue = NULL;
//		CC1101_init(&cc1101_hw);
//		if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 100) {
//			setIdleState();
//			flushRxFifo();
//			flushTxFifo();
//			CC1101_rx_mode();
//			mgr->enable_interrupt();
//			next_state = RADIO_STATE_IDLE;
//			dbg_printf("Radio init done\n");
//		}
		next_state = RADIO_STATE_RESET;
		break;
	}
	case RADIO_STATE_RESET:
		if (msg->type == NOSYS_MSG_STATE) {
			dbg_printf("Reset radio start\n");
			mgr->disable_interrupt();
			CC1101_init(&cc1101_hw);
			queue = NULL;
			current_packet = NULL;
		} else if (msg->type == NOSYS_TIMER_MSG) {
//			if (marcState == 0x01) {
			setRxState();
			byte marcState = (getMarcState() & 0x1F);
			dbg_printf("MARCSTATE in reset %d\n", marcState);
			dbg_printf("Reset done\n");
			mgr->enable_interrupt();
			next_state = RADIO_STATE_IDLE;
//			}
		}
//		}
		break;
	case RADIO_STATE_IDLE:
		if (msg->type == NOSYS_MSG_STATE) {
//			if ((getMarcState() & 0x1F) != 0x0D) {
//				setRxState();
//			} else {
//				dbg_printf("rx mode ok\n");
//			}
//			dbg_printf("RADIO_STATE_IDLE marcstate %x \n",
//					(getMarcState() & 0x1F));
			CC1101_rx_mode();

		} else if (msg->type == NOSYS_MSG_RADIO_NOTIFY) {
			next_state = RADIO_STATE_RX;
		} else if (current_packet) {
			next_state = RADIO_STATE_TX;
		}
//		else if(msg->type == NOSYS_TIMER_MSG && (getMarcState() & 0x1F) != 0x0D){
//			dbg_printf("We should enter rx mode %x\n",getMarcState() & 0x1F);
//			CC1101_rx_mode();
//		}
		break;
	case RADIO_STATE_RX:
		if (CC1101_receiveData(&in_packet)) {
//			DBG("Len:%d\nLQI %d\nRSSI %d\nCRC %d\n", in_packet.length,
//					in_packet.rssi, in_packet.lqi, in_packet.crc_ok);
			if (mgr->data_recieved && in_packet.crc_ok) {
				mgr->data_recieved((unsigned char *) &in_packet.data[0],
						(unsigned char) in_packet.length,
						(unsigned char) in_packet.rssi,
						(unsigned char) in_packet.lqi, mgr->userdata);
			}

		}

		next_state = RADIO_STATE_IDLE;
		break;
	case RADIO_STATE_TX: {
		if (msg->type == NOSYS_MSG_STATE) {
			if (current_packet == NULL) {
				DBG("SEND FAILED current_packet null\n");
				next_state = RADIO_STATE_IDLE;
				break;

			}
//			DBG("CC1101 send ...");
			if (!CC1101_sendData(current_packet->packet)) {
				dbg_printf("SEND FAILED send data1\n");
				next_state = RADIO_STATE_IDLE;
				if (current_packet->radio_send_done_fn) {
					current_packet->radio_send_done_fn(0,
							current_packet->userdata);
				}
				next_state = RADIO_STATE_IDLE;
				current_packet = NULL;
				break;
			}

		} else if (msg->type == NOSYS_MSG_RADIO_NOTIFY) {

			if (current_packet && current_packet->radio_send_done_fn) {
				current_packet->radio_send_done_fn(1, current_packet->userdata);
			}
//			CC1101_rx_mode();
//			flushTxFifo();
//			setRxState();

			next_state = RADIO_STATE_IDLE;
			current_packet = NULL;
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 100) {
//			setIdleState();
			flushTxFifo();
			setRxState();
			dbg_printf("CC1101 tx timeout/n");
			if (current_packet && current_packet->radio_send_done_fn) {
				current_packet->radio_send_done_fn(ERR_RADIO_TX_TIMEOUT,
						current_packet->userdata);
			}
			current_packet = NULL;
			next_state = RADIO_STATE_IDLE;
		}

		break;
	}
	}
	return next_state;
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
s32_t radio_send(unsigned char *buffer, int len,
		void (*radio_send_done_fn)(unsigned int res, void *userdata),
		void *userdata) {
//	struct packet_queue *pq = alloc_packet();
	if (current_packet != NULL || len < (CC1101_PKTLEN - 3)) {
//		DBG("Malloc failed\n");
		return -1;
	}
	current_packet = &packet_buf;
	memcpy(&current_packet->packet.data[0], buffer, len);

	current_packet->packet.length = len;
	current_packet->userdata = userdata;
	current_packet->radio_send_done_fn = radio_send_done_fn;
	current_packet->next = NULL;

//	if (queue) {
//		queue->next = pq;
//		queue = pq;
//	} else {
//		queue = pq;
//	}
	return ERR_OK;
}

void radio_link_status(struct rssi_lqi *status) {
	status->rssi = 0;
//	DBG("radio_link_status start\n");
	CC1101_readBurstReg((byte *) &status->rssi, CC1101_RSSI, 1);

//	DBG("radio_link_status 1\n");
	CC1101_readBurstReg(&status->lqi, CC1101_LQI, 1);

//	DBG("radio_link_status 2\n");
	if (status->rssi >= 128) {
		status->rssi = (((short) status->rssi - 256) / 2) - 74;
	} else {
		status->rssi = ((short) status->rssi / 2) - 74;
	}
}
void radio_notify(struct radio_mgr *mgr) {
	if (mgr->state == RADIO_STATE_TX) {
		DBG("CC1101 sent ok\n");

	} else if (mgr->state == RADIO_STATE_IDLE) {
		DBG("CC1101 recive ok\n");
		mgr->state = RADIO_STATE_RX;
	} else {
//		DBG("MISSED state %s\n",state[radio_mgr.state]);
	}
}
#ifdef LINUX
static void radio_mgr_timer_cb(void) {
	radio_fn();
}
#endif
void radio_init() {
	radio_mgr.state = RADIO_STATE_INIT;
//	memset(&packet_used, 0, PACKET_BUF_SIZE);
	current_packet = NULL;

#ifdef LINUX
	timer_add_callback(radio_mgr_timer_cb);
	int res = pthread_create(&t, NULL, radio_mgr_thread, (void *) &radio_mgr);

	if (res != 0) {
		perror(NULL);
	}
#endif
}

void radio_mgr_reset(void *mgr, void *reply_queue) {
	struct radio_mgr *radio_mgr = (struct radio_mgr *) mgr;
	struct nosys_msg *msg = nosys_queue_create_msg();
	msg->type = NOSYS_MSG_RADIO_RESET;
	msg->ptr = reply_queue;

	nosys_queue_add_last(radio_mgr->inq, msg);
}
void radio_fn() {
	struct radio_mgr *mgr = (struct radio_mgr *) self->user_data;
	struct nosys_msg *msg = nosys_msg_get();
	u8_t skip_state = 0;
	if (msg) {

		if (msg->type == NOSYS_TIMER_MSG) {

			mgr->time_in_state++;
			if (mgr->state == RADIO_STATE_IDLE) {
				radio_link_status(&status);
			}
		} else if (msg->type == NOSYS_MSG_RADIO_RESET) {
			mgr->state = RADIO_STATE_RESET;
			mgr->reset_queue = msg->ptr;
		}

		if (!skip_state) {
			while (1) {
				enum radio_state next_state = radio_state_machine(mgr, msg);
				if (next_state == mgr->state) {
					break;
				}
#ifdef STATE_DEBUG
				DBG("%s -> %s\n", state[mgr->state], state[next_state]);
#endif
				mgr->state = next_state;
				mgr->time_in_state = 0;
				msg->type = NOSYS_MSG_STATE;
			}
		}
		nosys_queue_msg_destroy(msg);

	}

}
