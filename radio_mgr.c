/*
 * radio.c
 *
 *  Created on: Feb 19, 2016
 *      Author: mattias
 */
#include <radio_mgr.h>
#include <string.h>
#include <cc1101.h>
#include <nosys_task.h>
#include <nosys_queue.h>
#include <error_codes.h>
struct radio_mgr radio_mgr;
static CCPACKET in_packet;
static struct packet_queue *current_packet;
static struct packet_queue packet_buf;
struct cc1101_hw cc1101_hw;
#define RESET_TIME_IN_STATE mgr->time_in_state = 0
static struct packet_queue *queue;
struct rssi_lqi status;
#ifdef STATE_DEBUG
static const char *state[] = { "RADIO_STATE_INIT", "RADIO_STATE_RESET",
		"RADIO_STATE_IDLE", "RADIO_STATE_RX", "RADIO_STATE_TX" };
#endif

enum radio_state radio_state_machine(struct radio_mgr *mgr,
		struct nosys_msg *msg) {
	enum radio_state next_state = mgr->state;
	switch (mgr->state) {
	case RADIO_STATE_INIT: {

		next_state = RADIO_STATE_RESET;
		break;
	}
	case RADIO_STATE_RESET:
		if (msg->type == NOSYS_MSG_STATE) {
			if (mgr->disable_interrupt) {
				mgr->disable_interrupt();

			}
			CC1101_init(&cc1101_hw);
			dbg_printf("Reset radio start\n");

			queue = NULL;
			current_packet = NULL;
		} else if (msg->type == NOSYS_TIMER_MSG) {
			byte marcState = (getMarcState() & 0x1F);
			if (marcState == 0x0D) {
				dbg_printf("MARCSTATE in reset %d\n", marcState);
				if (mgr->enable_interrupt) {
					mgr->enable_interrupt();
				}
				setRxState();
				dbg_printf("Reset done\n");
				if (mgr->reset_queue) {
					post_msg(mgr->reset_queue, NOSYS_MSG_RADIO_RESET_DONE);
				}
				next_state = RADIO_STATE_IDLE;
			}
		}
		break;
	case RADIO_STATE_IDLE:
		if (msg->type == NOSYS_MSG_STATE) {

			if ((getMarcState() % 0x1F) != 0x0D) {
				CC1101_rx_mode();
			}

		} else if (msg->type == NOSYS_MSG_RADIO_NOTIFY) {
			next_state = RADIO_STATE_RX;
		} else if (current_packet) {
			next_state = RADIO_STATE_TX;
		} else if (msg->type == NOSYS_TIMER_MSG
				&& (mgr->time_in_state % 100) == 0) {
			if ((getMarcState() & 0x1F) != 0x0D) {
				CC1101_rx_mode();
			}
		}
		break;
	case RADIO_STATE_RX:
		if (msg->type == NOSYS_MSG_STATE) {
			if (CC1101_receiveData(&in_packet)) {
				if (mgr->data_recieved && in_packet.crc_ok) {
					mgr->data_recieved((unsigned char *) &in_packet.data[0],
							(unsigned char) in_packet.length,
							(unsigned char) in_packet.rssi,
							(unsigned char) in_packet.lqi, mgr->userdata);
				}

			}
			next_state = RADIO_STATE_IDLE;
		}
		break;
	case RADIO_STATE_TX: {
		if (msg->type == NOSYS_MSG_STATE) {
			if (current_packet == NULL) {
				next_state = RADIO_STATE_IDLE;
				break;

			}
			if (!CC1101_sendData(current_packet->packet)) {
				if (current_packet->radio_send_done_fn) {
					current_packet->radio_send_done_fn(0,
							current_packet->userdata);
				}
				if ((getMarcState() & 0x1F) == 0x11) {
					next_state = RADIO_STATE_RX;
					break;
				}
			} else {
				if (current_packet && current_packet->radio_send_done_fn) {
					current_packet->radio_send_done_fn(1,
							current_packet->userdata);

				}

			}

			next_state = RADIO_STATE_IDLE;
			current_packet = NULL;
		}

		break;
	}
	}
	return next_state;
}

s32_t radio_send(unsigned char *buffer, int len,
		void (*radio_send_done_fn)(unsigned int res, void *userdata),
		void *userdata) {
	if (current_packet != NULL || len < (CC1101_PKTLEN - 3)) {
		return -1;
	}
	current_packet = &packet_buf;
	memcpy(&current_packet->packet.data[0], buffer, len);

	current_packet->packet.length = len;
	current_packet->userdata = userdata;
	current_packet->radio_send_done_fn = radio_send_done_fn;
	current_packet->next = NULL;

	return ERR_OK;
}

void radio_link_status(struct rssi_lqi *status) {
	memset(status, 0, sizeof(struct rssi_lqi));
	CC1101_readBurstReg((byte *) &status->rssi, CC1101_RSSI, 1);

	CC1101_readBurstReg(&status->lqi, CC1101_LQI, 1);

	if (status->rssi >= 128) {
		status->rssi = (((s16_t) status->rssi - 256) / 2) - 74;
	} else {
		status->rssi = ((s16_t) status->rssi / 2) - 74;
	}
}
void radio_notify(struct radio_mgr *mgr) {
	if (mgr->state == RADIO_STATE_TX) {

	} else if (mgr->state == RADIO_STATE_IDLE) {
		mgr->state = RADIO_STATE_RX;
	}
}
#ifdef LINUX
static void radio_mgr_timer_cb(void) {
	radio_fn();
}
#endif
void radio_init() {
	radio_mgr.state = RADIO_STATE_INIT;
	current_packet = NULL;

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
				if ((getMarcState() & 0x1F) == 0x11) {
					flushRxFifo();
					setRxState();
				}
			}
		} else if (msg->type == NOSYS_MSG_RADIO_RESET) {
			msg->type = NOSYS_MSG_STATE;
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
