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
//#define STATE_DEBUG
#ifdef STATE_DEBUG
static const char *state[] = {"RADIO_INIT", "RADIO_RESET",
	"RADIO_IDLE", "RADIO_RX", "RADIO_TX"};
#endif

static void radio_send_done_fn(s32_t res, void *userdata) {
	struct radio_mgr *mgr = (struct radio_mgr *) userdata;
	struct nosys_msg *msg = nosys_queue_create_msg();
	if (!msg)
		return;

	msg->type = NOSYS_MSG_RADIO_SEND_DONE;
	msg->value = res;
	nosys_queue_add_last(mgr->inq, msg);
}

static void radio_recieve_done_fn(s32_t res, void *userdata) {
	struct radio_mgr *mgr = (struct radio_mgr *) userdata;
	struct nosys_msg *msg = nosys_queue_create_msg();
	if (!msg)
		return;

	msg->type = NOSYS_MSG_RADIO_RECIEVED_DATA;
	msg->value = res;
	nosys_queue_add_last(mgr->inq, msg);

}
enum radio_state radio_state_machine(struct radio_mgr *mgr,
		struct nosys_msg *msg) {
	enum radio_state next_state = mgr->state;
	switch (mgr->state) {
	case RADIO_STATE_INIT: {
		break;
	}
	case RADIO_STATE_RESET:
		if (msg->type == NOSYS_MSG_STATE) {
			if (mgr->disable_interrupt) {
				mgr->disable_interrupt();

			}
			CC1101_init(&cc1101_hw);

			queue = NULL;
			if (current_packet != NULL) {
				memset(current_packet, 0, sizeof(struct packet_queue));
				current_packet = NULL;
			}
		} else if (msg->type == NOSYS_TIMER_MSG) {
			byte marcState = (getMarcState() & 0x1F);
			if (marcState == 0x0D) {
				if (mgr->enable_interrupt) {
					mgr->enable_interrupt();
				}
				setRxState();
				if (mgr->reset_queue) {
					post_msg(mgr->reset_queue, NOSYS_MSG_RADIO_RESET_DONE);
				}
				next_state = RADIO_STATE_IDLE;
			} else {
				setRxState();
			}
		}
		break;
	case RADIO_STATE_IDLE:
		if (msg->type == NOSYS_MSG_STATE) {
			if (mgr->enable_interrupt) {
				mgr->enable_interrupt();
			}
		}
		if (msg->type == NOSYS_MSG_RADIO_NOTIFY) {
			next_state = RADIO_STATE_RX;
		} else if (msg->type == NOSYS_MSG_RADIO_APP_SEND) {
			next_state = RADIO_STATE_TX;
		} else if (msg->type == NOSYS_TIMER_MSG) {
			if ((mgr->time_in_state % 10 == 0)
					&& ((getMarcState() & 0x1F) != 0x0D)) {
				next_state = RADIO_STATE_RESET;
			}
		}
		break;
	case RADIO_STATE_RX:
		if (msg->type == NOSYS_MSG_STATE) {
			memset(&in_packet, 0, sizeof(CCPACKET));
			if (CC1101_recieve(mgr->cc1101_mgr, &in_packet,
					radio_recieve_done_fn, mgr) != ERR_OK) {
				if (mgr->data_recieved) {
					mgr->data_recieved((unsigned char *) &in_packet.data[0],
							(unsigned char) in_packet.length,
							(unsigned char) in_packet.rssi,
							(unsigned char) in_packet.lqi, mgr->userdata);
				}
				next_state = RADIO_STATE_IDLE;

			} else {
				if (mgr->data_recieved) {
					mgr->data_recieved((unsigned char *) &in_packet.data[0],
							(unsigned char) in_packet.length,
							(unsigned char) in_packet.rssi,
							(unsigned char) in_packet.lqi, mgr->userdata);
				}

			}
		} else if (msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA) {
			if (mgr->data_recieved) {
				mgr->data_recieved((unsigned char *) &in_packet.data[0],
						(unsigned char) in_packet.length,
						(unsigned char) in_packet.rssi,
						(unsigned char) in_packet.lqi, mgr->userdata);
			}
			next_state = RADIO_STATE_IDLE;

		}
		break;
	case RADIO_STATE_TX: {
		if (msg->type == NOSYS_MSG_STATE) {
			if (mgr->disable_interrupt) {
				mgr->disable_interrupt();

			}

			if (current_packet == NULL) {
				next_state = RADIO_STATE_RESET;
				break;
			}
			if (CC1101_send(mgr->cc1101_mgr, &current_packet->packet,
					radio_send_done_fn, mgr) != ERR_OK) {
				if (current_packet->radio_send_done_fn) {
					current_packet->radio_send_done_fn(0,
							current_packet->userdata);

					memset(current_packet, 0, sizeof(struct packet_queue));
					current_packet = NULL;
				}
				next_state = RADIO_STATE_IDLE;

			}
		} else if (msg->type == NOSYS_MSG_RADIO_SEND_DONE) {
			u8_t res = 0;
			if (msg->value == ERR_OK) {
				res = 1;
			}
			if (current_packet->radio_send_done_fn) {
				current_packet->radio_send_done_fn(res,
						current_packet->userdata);

			}
			if(current_packet)
				memset(current_packet, 0, sizeof(struct packet_queue));

			current_packet = NULL;

			next_state = RADIO_STATE_IDLE;
		}

		break;
	}
	}
	return next_state;
}

s32_t radio_send(struct radio_mgr *mgr, unsigned char *buffer, int len,
		void (*radio_send_done_fn)(unsigned int res, void *userdata),
		void *userdata) {
	if (current_packet != NULL || len < (CC1101_PKTLEN - 3) || mgr->state != RADIO_STATE_IDLE) {
		return -1;
	}
	current_packet = &packet_buf;
	memset(current_packet, 0, sizeof(struct packet_queue));
	memcpy(&current_packet->packet.data[0], buffer, len);

	current_packet->packet.length = len;
	current_packet->userdata = userdata;
	current_packet->radio_send_done_fn = radio_send_done_fn;
	current_packet->next = NULL;

	struct nosys_msg *msg = nosys_queue_create_msg();

	if(msg == NULL) return -1;

	msg->type = NOSYS_MSG_RADIO_APP_SEND;
	nosys_queue_add_last(mgr->inq,msg);
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
	if (mgr->state == RADIO_STATE_IDLE) {
		mgr->state = RADIO_STATE_RX;
	}
}

void radio_init() {
	radio_mgr.state = RADIO_STATE_INIT;
	current_packet = NULL;

}

void radio_mgr_reset(void *mgr, void *reply_queue) {
	struct radio_mgr *radio_mgr = (struct radio_mgr *) mgr;
	struct nosys_msg *msg = nosys_queue_create_msg();
	if (!msg)
		return;

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
			if (mgr->time_in_state < 0xFFFF) {
				mgr->time_in_state++;
				cc1101_mgr_tick(mgr->cc1101_mgr);
			}

		} else if (msg->type == NOSYS_MSG_RADIO_RESET) {
			msg->type = NOSYS_MSG_STATE;
			mgr->state = RADIO_STATE_RESET;
		} else if (mgr->time_in_state > 10 && mgr->state != RADIO_STATE_IDLE) {
			msg->type = NOSYS_MSG_STATE;
			mgr->state = RADIO_STATE_RESET;

		}

		if (!skip_state) {
			while (1) {
				enum radio_state next_state = radio_state_machine(mgr, msg);
				if (next_state == mgr->state) {
					break;
				}
#ifdef STATE_DEBUG
				dbg_printf("%s -> %s\n", state[mgr->state], state[next_state]);
#endif
				mgr->state = next_state;
				mgr->time_in_state = 0;
				msg->type = NOSYS_MSG_STATE;
			}
		}

		nosys_queue_msg_destroy(msg);

	}

}
