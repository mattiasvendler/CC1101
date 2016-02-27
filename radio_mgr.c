/*
 * radio.c
 *
 *  Created on: Feb 19, 2016
 *      Author: mattias
 */
#include <radio_mgr.h>
#include <timer.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <cc1101.h>
#include <stdlib.h>
struct radio_mgr radio_mgr;
static pthread_t t;
static CCPACKET packet;
static CCPACKET in_packet;
static struct packet_queue *current_packet;
static unsigned int packet_flags = 0;
#define PACKET_SENT_OK 0x01
#define SENDING 0x02
#define RX_ENTERED 0x03
static const char *state[] = {	"RADIO_STATE_INIT",
		"RADIO_STATE_IDLE",
		"RADIO_STATE_RX",
		"RADIO_STATE_TX"
};
struct packet_queue {
	CCPACKET packet;
	void *userdata;
	void (*radio_send_done_fn)(unsigned int res, void *userdata);
	struct packet_queue *next;
};

static struct packet_queue *queue;
static void *statemachine(void *arg) {
	struct radio_mgr *mgr = (struct radio_mgr *) arg;
	while (1) {

		switch (mgr->state) {
		case RADIO_STATE_INIT: {
			int i;
			for (i = 0; i < 20; i++) {
				packet.data[i] = i + 1;
			}
			packet.length = 20;
			queue = NULL;
			mgr->state = RADIO_STATE_IDLE;
			break;
		}
		case RADIO_STATE_IDLE:
			if(!(packet_flags & RX_ENTERED) ){
				printf("%s\n",state[mgr->state]);
				if (CC1101_rx_mode())
					packet_flags |= RX_ENTERED;
				packet_flags &= (SENDING | PACKET_SENT_OK);
			}
			if (queue) {
//				printf("RADIO_STATE_IDLE\n");
				packet_flags &= ~RX_ENTERED;
				mgr->state = RADIO_STATE_TX;
			}
			break;
		case RADIO_STATE_RX:
			printf("%s\n",state[mgr->state]);
			if (CC1101_receiveData(&in_packet)) {
				printf("Len:%d\nLQI %d\n RSSI %d\n", in_packet.length,
						in_packet.lqi, in_packet.rssi);
				radio_mgr.state = RADIO_STATE_IDLE;
			}
			break;
		case RADIO_STATE_TX: {
			printf("%s\n",state[mgr->state]);
			if(!(packet_flags & SENDING) ){
//				printf("SEND START\n");
				packet_flags&=!RX_ENTERED;
				current_packet = queue;
				if(!CC1101_sendData(current_packet->packet)){
					radio_mgr.state=RADIO_STATE_IDLE;
				}else{
//					printf("WAIT FOR INTERRUPT\n");
					packet_flags|=SENDING;
				}
				queue = current_packet->next;
			} else if (packet_flags & PACKET_SENT_OK) {


				if (current_packet->radio_send_done_fn) {
					current_packet->radio_send_done_fn(CC1101_tx_fifo_empty(),
							current_packet->userdata);
				}
				mgr->state = RADIO_STATE_IDLE;
				free(current_packet);
				packet_flags &= !SENDING;
			}

			break;
		}
		}

		const struct timespec ts = { 0, 1000000 };
		struct timespec rem;
		sched_yield();
		nanosleep(&ts, &rem);
	}
	return arg;
}

void radio_send(unsigned char *buffer, int len,
		void (*radio_send_done_fn)(unsigned int res, void *userdata),
		void *userdata) {
//	printf("Paket len %d\n",len);
//	int i;
//	for(i=0;i<len;i++){
//		printf("%02X",buffer[i]);
//	}
//	printf("\n");
	struct packet_queue *pq = malloc(sizeof(struct packet_queue));
	memset(pq, 0, sizeof(struct packet_queue));

	memcpy(&pq->packet.data[0], buffer, len);
	pq->packet.length = len;
	pq->userdata = userdata;
	pq->radio_send_done_fn = radio_send_done_fn;
	pq->next = NULL;
	if (queue) {
		queue->next = pq;
		queue = pq;
	} else {
		queue = pq;
	}
}
void radio_notify() {
	if(radio_mgr.state == RADIO_STATE_TX && (packet_flags & SENDING)){
		packet_flags|= PACKET_SENT_OK;
	}else if(radio_mgr.state==RADIO_STATE_IDLE){
		radio_mgr.state=RADIO_STATE_RX;
	}else{
		printf("MISSED \n");
	}
}
static void radio_mgr_timer_cb(void) {
	radio_fn();
}
void radio_init() {
	radio_mgr.state = RADIO_STATE_IDLE;
	timer_add_callback(radio_mgr_timer_cb);
	int res = pthread_create(&t, NULL, statemachine, (void *) &radio_mgr);

	if (res != 0) {
		perror(NULL);
	}
}
void radio_fn() {

}
