/*
 * radio_msg_mgr.c
 *
 *  Created on: Mar 10, 2016
 *      Author: mattias
 */
#include <radio_msg_mgr.h>
#include <nosys_queue.h>
#include <nosys_task.h>
#include <nosys_timer.h>
#include <error_codes.h>
#include <types.h>
#include <debug.h>
#include <radio_mgr.h>
#include <string.h>
#include <radio_msg_parser.h>
#include <radio_msg.h>
static struct nosys_timer timer;
static u8_t lbt_fail = 0;
static u8_t rx_buff[61];
static volatile int rx_len;
static struct radio_msg_send_queue *tx_q;
static struct radio_msg_send_queue *ack_q;
static struct radio_msg_send_queue *tx_curr;
static struct radio_msg_send_queue msg_buf[RADIO_MSG_SEND_QUEUE_SIZE];
static u8_t msg_buf_size = RADIO_MSG_SEND_QUEUE_SIZE;
static u8_t msg_free[RADIO_MSG_SEND_QUEUE_SIZE];
#define RSSI_OK (-84)
#ifdef STATE_DEBUG
static const char *states[] = {"INIT", "RESET", "IDLE", "RX", "RX_ACK",
	"RX_ACK_DONE", "TX", "TX_DONE", "TX_ACK", "LBT"

};
#endif
extern struct rssi_lqi status;

static struct radio_msg_send_queue * alloc_msg(void) {
	struct radio_msg_send_queue *msg = NULL;
	u8_t i;
	for (i = 0; i < RADIO_MSG_SEND_QUEUE_SIZE; i++) {
		if (msg_free[i] == 0) {
			msg = &msg_buf[i];
			memset(msg, 0, sizeof(struct radio_msg_send_queue));
			msg_free[i] = 1;
			msg_buf_size--;
			break;
		}
	}
	if (msg == NULL) {
		dbg_printf("alloc_msg failed\n");
	}
	return msg;
}

static void free_msg(struct radio_msg_send_queue *msg) {
	u8_t i;
	for (i = 0; i < RADIO_MSG_SEND_QUEUE_SIZE; i++) {
		if (&msg_buf[i] == msg) {
			msg_free[i] = 0;
			memset(&msg_buf[i], 0, sizeof(struct radio_msg_send_queue));
			msg_buf_size++;
			break;
		}
	}
}
static void radio_msg_mgr_timer_cb(void *userdata) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) userdata;
	struct nosys_msg *msg = nosys_queue_create_msg();
	msg->type = NOSYS_TIMER_MSG;
	nosys_queue_add_last(mgr->inq, msg);
}
static void radio_send_done_fn(unsigned int res, void *userdata) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) userdata;
	struct nosys_msg *msg = nosys_queue_create_msg();
	msg->type = NOSYS_MSG_RADIO_SEND_DONE;
	msg->value = (u32_t) res;
	nosys_queue_add_last(mgr->inq, msg);
}
static void radio_send_ack_done_fn(unsigned int res, void *userdata) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) userdata;
	struct nosys_msg *msg = nosys_queue_create_msg();
	msg->type = NOSYS_MSG_RADIO_APP_SEND_ACK;
	msg->value = (u32_t) res;
	nosys_queue_add_last(mgr->inq, msg);
}
static void print_packege(struct radio_packet_header *h) {
#ifdef STATE_DEBUG
	dbg_printf("Package:");
	dbg_printf("SOURCE: %x\n", h->source);
	dbg_printf("TARGET: %x\n", h->target);
	dbg_printf("SEQ %d\n", htons(h->seq));
	dbg_printf("CRC %x\n", h->crc);
	dbg_printf("length %d\n", h->length);
	dbg_printf("flags %x\n", h->flags);
	dbg_printf("Type: %x\n", (h->flags & 0x60) >> 5);
	dbg_printf("ack %s\n", ((h->flags & 0x80) > 0 ? "TRUE" : "FALSE"));
	dbg_printf("msg %x\n", h->msg_type);
#endif
}
void radio_msg_mgr_data_recieved_cb(unsigned char *data, unsigned char len,
		unsigned char rssi, unsigned char lqi, void *userdata) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) userdata;
	if (rx_len == 0) {
		struct nosys_msg *msg = nosys_queue_create_msg();
		if (msg) {
			msg->type = NOSYS_MSG_RADIO_RECIEVED_DATA;
			msg->extra_data = len;
			rx_len = len;
			memcpy(&rx_buff[0], data, len);
			nosys_queue_add_last(mgr->inq, msg);
		}
	}
}

void radio_msg_mgr_init(struct radio_msg_mgr *mgr, u32_t local_address) {
	mgr->state = RADIO_MSG_MGR_STATE_INIT;
	mgr->time_in_state = 0;
	mgr->local_address = local_address;
	timer.cb = radio_msg_mgr_timer_cb;
	timer.flags = NOSYS_TIMER_CONTINIOUS | NOSYS_TIMER_ACITVE;
	timer.q = mgr->inq;
	timer.ticks = 0;
	timer.timeout = 1;
	timer.user_data = mgr;
	nosys_timer_add(&timer);
	tx_q = NULL;
	ack_q = NULL;
	tx_curr = NULL;
	memset(&msg_free[0], 0, sizeof(msg_free));
	memset(&msg_buf[0], 0, sizeof(msg_buf));

}
s32_t radio_msg_send(void *data, u8_t len, u8_t require_ack, void *userdata,
		void (*send_done_cb)(s32_t res, void *userdata)) {

	struct radio_msg_send_queue *q = alloc_msg();
	if (q) {
		memcpy(&q->data[0], data, len);
		q->userdata = userdata;
		q->send_done_cb = send_done_cb;
		q->len = len;
		q->require_ack = require_ack;
		q->next = NULL;
		if (tx_q == NULL) {
			tx_q = q;
		} else {
			return -1;
		}
	} else {
		return -1;
	}
	return ERR_OK;
}
static enum radio_msg_mgr_state radio_msg_mgr_statemachine(
		struct radio_msg_mgr *mgr, struct nosys_msg *msg) {
	enum radio_msg_mgr_state next_state = mgr->state;
	switch (mgr->state) {
	case RADIO_MSG_MGR_STATE_INIT:
		next_state = RADIO_MSG_MGR_STATE_RESET;
		break;
	case RADIO_MSG_MGR_STATE_RESET:
		if (msg->type == NOSYS_MSG_STATE) {
			radio_mgr_reset(mgr->radio_mgr, mgr->inq);
			free_msg(tx_curr);
			tx_curr = NULL;
			rx_len = 0;
		} else if (msg->type == NOSYS_MSG_RADIO_RESET_DONE) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 3000) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		}
		break;
	case RADIO_MSG_MGR_STATE_IDLE:
		if (msg->type == NOSYS_MSG_STATE) {
			rx_len = 0;
		} else if (msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA) {
				next_state = RADIO_MSG_MGR_STATE_RX;
		} else if ((msg->type == NOSYS_TIMER_MSG)
				&& ((tx_q != NULL && tx_curr == NULL) || tx_curr != NULL)) {
			next_state = RADIO_MSG_MGR_STATE_LBT;
		}
		break;
	case RADIO_MSG_MGR_STATE_RX:
		if (msg->type == NOSYS_MSG_STATE) {
			u8_t *msg_data = &rx_buff[sizeof(struct radio_packet_header)];
			struct radio_packet_header *h =
					(struct radio_packet_header *) &rx_buff[0];
			print_packege(h);
			if (htonl(h->target) == mgr->local_address) {
				s32_t res = ERR_OK;
				if (mgr->handle_ctrl_msg_cb
						&& (((h->flags & 0x60) >> 5) & RADIO_MSG_CTRL) > 0) {
					res = mgr->handle_ctrl_msg_cb(h->source, h->msg_type,
							msg_data, rx_len - 14, mgr->user);
				}
				if (((h->flags & 0x10) != 0) && (res == ERR_OK)) {
					next_state = RADIO_MSG_MGR_STATE_RX_ACK;
					break;
				}
			}
			//Should not be acked
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		}

		break;

	case RADIO_MSG_MGR_STATE_RX_ACK:
		if (msg->type == NOSYS_MSG_STATE) {
			struct radio_packet_header *h =
					(struct radio_packet_header *) &rx_buff[0];
			u32_t s = h->target;
			h->target = h->source;
			h->source = s;
			h->length = 0;
			h->flags |= 0x80;
			h->flags &= ~0x10;
			print_packege(h);
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 10) {
			if (radio_send(&rx_buff[0], sizeof(struct radio_packet_header),
					radio_send_ack_done_fn, mgr) == ERR_OK) {
				next_state = RADIO_MSG_MSG_STATE_RX_ACK_DONE;
			} else {
				next_state = RADIO_MSG_MGR_STATE_IDLE;
			}
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 100) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		}
		break;
	case RADIO_MSG_MSG_STATE_RX_ACK_DONE:
		if (msg->type == NOSYS_MSG_RADIO_APP_SEND_ACK) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 100) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		}

		break;
	case RADIO_MSG_MGR_STATE_TX:
		if (msg->type == NOSYS_MSG_STATE) {
			lbt_fail = 0;
			if (tx_curr == NULL) {
				if (tx_q != NULL) {
					tx_curr = tx_q;
					tx_q = tx_q->next;
				} else {
					next_state = RADIO_MSG_MGR_STATE_IDLE;
					break;
				}
			}
			if (tx_curr != NULL) {
				if (radio_send(&tx_curr->data[0], tx_curr->len,
						radio_send_done_fn, mgr) == ERR_OK) {
					struct radio_packet_header *tx_header =
										(struct radio_packet_header *) &tx_curr->data[0];
					print_packege(tx_header);
					next_state = RADIO_MSG_MGR_STATE_TX_DONE;
				} else {
					next_state = RADIO_MSG_MGR_STATE_IDLE;
				}

			} else {
				next_state = RADIO_MSG_MGR_STATE_IDLE;
				tx_curr = NULL;
			}
		}
		break;
	case RADIO_MSG_MGR_STATE_TX_DONE:
		if (msg->type == NOSYS_MSG_RADIO_SEND_DONE) {
			if (msg->value == ERR_RADIO_TX_TIMEOUT) {
				next_state = RADIO_MSG_MGR_STATE_LBT;
			} else if (tx_curr != NULL && tx_curr->require_ack) {
				next_state = RADIO_MSG_MGR_STATE_TX_ACK;
			} else {
				if (tx_curr && tx_curr->send_done_cb) {
					tx_curr->send_done_cb(msg->value, tx_curr->userdata);
					free_msg(tx_curr);
					tx_curr = NULL;
				}
				next_state = RADIO_MSG_MGR_STATE_IDLE;
			}
		} else if (mgr->time_in_state >= 100) {
			if (tx_curr->resends < 5) {
				next_state = RADIO_MSG_MGR_STATE_LBT;
				tx_curr->resends++;
			} else {
				dbg_printf("TX SEND DONE timeout\n");
				if (tx_curr->send_done_cb) {
					tx_curr->send_done_cb(0, tx_curr->userdata);
				}
				if (tx_curr) {
					free_msg(tx_curr);
					tx_curr = NULL;
				}
				next_state = RADIO_MSG_MGR_STATE_IDLE;
			}
		}

		break;
	case RADIO_MSG_MGR_STATE_TX_ACK:
		if (msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA) {

			struct radio_packet_header *h =
					(struct radio_packet_header *) &rx_buff[0];
			struct radio_packet_header *tx_header =
					(struct radio_packet_header *) &tx_curr->data[0];
			print_packege(h);
			if (h->target == htonl(mgr->local_address) && h->seq == tx_header->seq
					&& (h->flags & 0x80) > 0) {
				if (tx_curr->send_done_cb) {
					tx_curr->send_done_cb(1, tx_curr->userdata);
				}
				if (tx_curr) {
					free_msg(tx_curr);
					tx_curr = NULL;
				}
				next_state = RADIO_MSG_MGR_STATE_IDLE;
				break;
			}

		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state >= 100) {
			if (tx_curr->resends <= 5) {
				next_state = RADIO_MSG_MGR_STATE_LBT;
				tx_curr->resends++;
			} else {
				dbg_printf("TX ACK timeout\n");
				if (tx_curr->send_done_cb) {
					tx_curr->send_done_cb(0, tx_curr->userdata);
				}
				if (tx_curr) {
					free_msg(tx_curr);
					tx_curr = NULL;
				}
				next_state = RADIO_MSG_MGR_STATE_IDLE;
			}
		}
		break;
	case RADIO_MSG_MGR_STATE_LBT:
		if (msg->type == NOSYS_MSG_STATE) {
			radio_link_status(&status);
			if (status.rssi <= RSSI_OK) {
				dbg_printf("LBT ok rssi=%d\n", status.rssi);
				lbt_fail = 0;
				next_state = RADIO_MSG_MGR_STATE_TX;
			}
			rx_len = 0;
		} else if (msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA) {
			lbt_fail = 0;
			next_state = RADIO_MSG_MGR_STATE_RX;
		} else if (msg->type == NOSYS_TIMER_MSG) {
			radio_link_status(&status);
			if (status.rssi <= RSSI_OK) {
				dbg_printf("LBT ok rssi=%d\n", status.rssi);
				lbt_fail = 0;
				next_state = RADIO_MSG_MGR_STATE_TX;
			} else if (mgr->time_in_state > 100) {
				radio_link_status(&status);
				lbt_fail++;
				dbg_printf("LBT fail rssi %d count %d\n", status.rssi,
						lbt_fail);
				if (lbt_fail > 10) {
					lbt_fail = 0;
					next_state = RADIO_MSG_MGR_STATE_RESET;
				} else {
					next_state = RADIO_MSG_MGR_STATE_IDLE;
				}
			}
		}
		break;
	}
	return next_state;

}
void radio_msg_mgr_fn(void) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) self->user_data;
	struct nosys_msg *msg = nosys_msg_get();
	u8_t skip_state = 0;
	if (msg->type == NOSYS_TIMER_MSG) {
		mgr->time_in_state++;

	}
//	else if (msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA
//			&& !(mgr->state == RADIO_MSG_MGR_STATE_TX_ACK
//					|| mgr->state == RADIO_MSG_MGR_STATE_IDLE)) {
//		nosys_msg_postpone(mgr->inq, msg);
//		skip_state = 1;
//	}
	if (msg) {
		if (!skip_state) {
			while (1) {
				enum radio_msg_mgr_state next_state =
						radio_msg_mgr_statemachine(mgr, msg);
				if (mgr->state != next_state) {
#ifdef STATE_DEBUG
					dbg_printf("%s -> %s\n", states[mgr->state],
							states[next_state]);
#endif
					msg->type = NOSYS_MSG_STATE;
					mgr->time_in_state = 0;
					mgr->state = next_state;
				} else {
					break;
				}
			}
		}
		nosys_queue_msg_destroy(msg);
	}

}

