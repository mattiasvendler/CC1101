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
#define RX_BUF_SIZE (61)
static struct nosys_timer timer;
static u8_t lbt_fail = 0;
static u8_t rx_buff[RX_BUF_SIZE];
static volatile int rx_len;
static struct radio_msg_mgr *local_mgr;
static struct radio_msg_send_queue *tx_curr;
static struct radio_msg_send_queue tx_curr_buf;
#define RSSI_OK (-84)
//#define STATE_DEBUG
#ifdef STATE_DEBUG
static const char *states[] = { "INIT", "RESET", "IDLE", "RX", "RX_ACK",
		"RX_ACK_DONE", "TX", "TX_DONE", "TX_ACK", "LBT"

};
#endif
extern struct rssi_lqi status;

static void radio_msg_mgr_timer_cb(void *userdata) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) userdata;
	struct nosys_msg *msg = nosys_queue_create_msg();

	if (!msg)
		return;

	msg->type = NOSYS_TIMER_MSG;
	nosys_queue_add_last(mgr->inq, msg);
}
static void radio_send_done_fn(unsigned int res, void *userdata) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) userdata;
	struct nosys_msg *msg = nosys_queue_create_msg();

	if (!msg)
		return;

	msg->type = NOSYS_MSG_RADIO_SEND_DONE;
	msg->value = (u32_t) res;
	nosys_queue_add_last(mgr->inq, msg);
}
static void radio_send_ack_done_fn(unsigned int res, void *userdata) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) userdata;
	struct nosys_msg *msg = nosys_queue_create_msg();

	if (msg == NULL)
		return;

	msg->type = NOSYS_MSG_RADIO_APP_SEND_ACK;
	msg->value = (u32_t) res;
	nosys_queue_add_last(mgr->inq, msg);
}
#ifdef STATE_DEBUG
static void print_packege(struct radio_packet_header *h) {
//	dbg_printf("Package:");
//	dbg_printf("SOURCE: %d\n", h->source);
//	dbg_printf("TARGET: %d\n", h->target);
//	dbg_printf("SEQ %d\n", h->seq);
//	dbg_printf("CRC %x\n", h->crc);
//	dbg_printf("length %d\n", h->length);
//	dbg_printf("flags %x\n", h->flags);
//	dbg_printf("Type: %x\n", (h->flags & 0x60) >> 5);
//	dbg_printf("ack %s\n", ((h->flags & 0x80) > 0 ? "TRUE" : "FALSE"));
//	dbg_printf("msg %x\n", h->msg_type);
}
#endif
void radio_msg_mgr_data_recieved_cb(unsigned char *data, unsigned char len,
		unsigned char rssi, unsigned char lqi, void *userdata) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) userdata;
	if (rx_len == 0
			&& (len >= sizeof(struct radio_packet_header) && len < 61)) {
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
void radio_msg_mgr_reset(struct radio_msg_mgr *mgr) {
	mgr->state = RADIO_MSG_MGR_STATE_INIT;
	mgr->time_in_state = 0;

}
void radio_msg_mgr_init(struct radio_msg_mgr *mgr, u32_t local_address,
		u8_t handle_broadcast) {
	mgr->state = RADIO_MSG_MGR_STATE_INIT;
	mgr->time_in_state = 0;
	mgr->local_address = local_address;
	mgr->handle_broadcast = handle_broadcast;
	timer.cb = radio_msg_mgr_timer_cb;
	timer.flags = NOSYS_TIMER_CONTINIOUS | NOSYS_TIMER_ACITVE;
	timer.q = mgr->inq;
	timer.ticks = 0;
	timer.timeout = 1;
	timer.user_data = mgr;
	nosys_timer_add(&timer);
	tx_curr = NULL;
	local_mgr = mgr;
}
s32_t radio_msg_send(void *data, u8_t len, u8_t require_ack, void *userdata,
		void (*send_done_cb)(s32_t res, void *userdata),
		struct radio_msg_mgr *radio_mgr) {

	if (tx_curr == NULL && radio_mgr->state == RADIO_MSG_MGR_STATE_IDLE) {
		tx_curr = &tx_curr_buf;
		memset(&rx_buff[0], 0, RX_BUF_SIZE);
		memset(tx_curr, 0, sizeof(struct radio_msg_send_queue));
		memcpy(&tx_curr->data[0], data, len);
		tx_curr->userdata = userdata;
		tx_curr->send_done_cb = send_done_cb;
		tx_curr->len = len;
		tx_curr->require_ack = require_ack;
		tx_curr->next = NULL;
		tx_curr->resends = 0;
		tx_curr->result = 0;

		struct nosys_msg *msg = nosys_queue_create_msg();
		if (msg == NULL)
			return -1;

		msg->type = NOSYS_MSG_RADIO_APP_SEND;
		nosys_queue_add_last(radio_mgr->inq, msg);
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
			if (tx_curr != NULL) {
				if (mgr->radio_recieved_cb) {
					mgr->radio_recieved_cb(tx_curr->len, &tx_curr->data[0],
							mgr->user);
				}
				if (tx_curr->send_done_cb)
					tx_curr->send_done_cb(0, tx_curr->userdata);
			}
			dbg_printf("RADIO_MSG_MGR_STATE_RESET\n");
//			radio_mgr_reset(mgr->radio_mgr, mgr->inq);

			memset(&rx_buff[0], 0, RX_BUF_SIZE);
			memset(&tx_curr_buf, 0, sizeof(struct radio_msg_send_queue));
			rx_len = 0;
			lbt_fail = 0;
			tx_curr = NULL;
			mgr->fail_count=0;
			mgr->rx_led_on();
			mgr->tx_led_on();
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		} else if (msg->type == NOSYS_MSG_RADIO_RESET_DONE) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 300) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		} else if (msg->type == NOSYS_TIMER_MSG
				&& mgr->time_in_state % 200 == 0) {
			dbg_printf("RESET Tick\n");
		}
		break;
	case RADIO_MSG_MGR_STATE_IDLE:
		if (msg->type == NOSYS_MSG_STATE) {
			if (mgr->tx_led_off)
				mgr->tx_led_off();

			if (mgr->rx_led_off)
				mgr->rx_led_off();

			if (tx_curr != NULL) {
				if(tx_curr->result == 0){
					mgr->fail_count++;
					if(mgr->fail_count >= 3){
						mgr->fail_count = 0;
						next_state = RADIO_MSG_MGR_STATE_RESET;
					}
				}else{
					mgr->fail_count = 0;
				}
				if (mgr->radio_recieved_cb) {
					mgr->radio_recieved_cb(tx_curr->len, &tx_curr->data[0],
							mgr->user);
				}

				if (tx_curr->send_done_cb)
					tx_curr->send_done_cb(tx_curr->result, tx_curr->userdata);
			}

			if (rx_len > 0 && rx_len >=sizeof(struct radio_packet_header) && mgr->radio_recieved_cb) {
				struct radio_packet_header *h =
						(struct radio_packet_header *) &rx_buff[0];
				if (h->length
						< (RX_BUF_SIZE - sizeof(struct radio_packet_header))) {
					mgr->radio_recieved_cb(
							sizeof(struct radio_packet_header) + h->length,
							&rx_buff[0], mgr->user);
				}
			}

			memset(&rx_buff[0], 0, RX_BUF_SIZE);
			memset(&tx_curr_buf, 0, sizeof(struct radio_msg_send_queue));
			rx_len = 0;
			lbt_fail = 0;
			tx_curr = NULL;
		} else if (msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA) {
			lbt_fail = 0;
			next_state = RADIO_MSG_MGR_STATE_RX;
		} else if ((msg->type == NOSYS_MSG_RADIO_APP_SEND)
				&& (tx_curr != NULL )) {
			next_state = RADIO_MSG_MGR_STATE_LBT;
		}
		break;
	case RADIO_MSG_MGR_STATE_RX:
		if (msg->type == NOSYS_MSG_STATE) {
			if (mgr->rx_led_on)
				mgr->rx_led_on();

			struct radio_packet_header *h =
					(struct radio_packet_header *) &rx_buff[0];
#ifdef STATE_DEBUG
			print_packege(h);
#endif
			if ((rx_len - sizeof(struct radio_packet_header) >= 0)
					&& h->length <= (RX_BUF_SIZE - sizeof(struct radio_packet_header))) {

				u8_t *msg_data = &rx_buff[sizeof(struct radio_packet_header)];

				if ((h->target == mgr->local_address
						&& mgr->node_allowed(h->source))
						|| (h->target == 0xFFFFFFFF)) {

					s32_t res = ERR_OK;

					if (mgr->handle_ctrl_msg_cb) {
						res = mgr->handle_ctrl_msg_cb(h->source, h->msg_type,
								msg_data,
								rx_len - sizeof(struct radio_packet_header),
								mgr->user);
					}
					if (((h->flags & 0x10) != 0) && (res == ERR_OK)) {
						next_state = RADIO_MSG_MGR_STATE_RX_ACK;
					} else {
						//Should not be acked
						next_state = RADIO_MSG_MGR_STATE_IDLE;
					}
					if (rx_len > 0 && mgr->radio_recieved_cb) {
						struct radio_packet_header *h =
								(struct radio_packet_header *) &rx_buff[0];
						if (h->length
								< (RX_BUF_SIZE
										- sizeof(struct radio_packet_header))) {
							mgr->radio_recieved_cb(
									sizeof(struct radio_packet_header)
											+ h->length, &rx_buff[0],
									mgr->user);
						}
					}

				}

			}

		}else if(msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 50){
			next_state = RADIO_MSG_MGR_STATE_RESET;
		}

		break;

	case RADIO_MSG_MGR_STATE_RX_ACK:
		if (msg->type == NOSYS_MSG_STATE) {
			if (mgr->tx_led_on)
				mgr->tx_led_on();
			if (mgr->rx_led_off)
				mgr->rx_led_off();
			if(rx_len < sizeof(struct radio_packet_header)){
				rx_len = 0;
				next_state = RADIO_MSG_MGR_STATE_IDLE;
				break;
			}

			struct radio_packet_header *h =
					(struct radio_packet_header *) &rx_buff[0];
			u32_t s = h->target;
			h->target = h->source;
			h->source = s;
			h->length = 0;
			h->flags |= 0x80;
			h->flags &= ~0x10;
#ifdef STATE_DEBUG
			print_packege(h);
#endif
			if (radio_send(mgr->radio_mgr, &rx_buff[0],
					sizeof(struct radio_packet_header), radio_send_ack_done_fn,
					mgr) != ERR_OK) {
				next_state = RADIO_MSG_MGR_STATE_IDLE;
			}
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 10) {
			next_state = RADIO_MSG_MGR_STATE_RESET;
		} else if (msg->type == NOSYS_MSG_RADIO_APP_SEND_ACK) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		}
		break;
	case RADIO_MSG_MSG_STATE_RX_ACK_DONE:
		if (msg->type == NOSYS_MSG_RADIO_APP_SEND_ACK) {
			next_state = RADIO_MSG_MGR_STATE_IDLE;
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 50) {
			next_state = RADIO_MSG_MGR_STATE_RESET;
		}

		break;
	case RADIO_MSG_MGR_STATE_TX:
		if (msg->type == NOSYS_MSG_STATE) {
			if (mgr->tx_led_on)
				mgr->tx_led_on();


			lbt_fail = 0;
			if (radio_send(mgr->radio_mgr, &tx_curr->data[0], tx_curr->len,
					radio_send_done_fn, mgr) == ERR_OK) {
				next_state = RADIO_MSG_MGR_STATE_TX_DONE;
			} else {
				tx_curr->result = 0;
				next_state = RADIO_MSG_MGR_STATE_IDLE;
			}

		}else if(msg->type == NOSYS_TIMER_MSG && mgr->time_in_state > 50){
			next_state = RADIO_MSG_MGR_STATE_RESET;
		}
		break;
	case RADIO_MSG_MGR_STATE_TX_DONE:
		if (msg->type == NOSYS_MSG_RADIO_SEND_DONE) {
			if (!msg->value) {
				tx_curr->result = 0;
				next_state = RADIO_MSG_MGR_STATE_IDLE;
			} else if (tx_curr != NULL && tx_curr->require_ack) {
				next_state = RADIO_MSG_MGR_STATE_TX_ACK;
			} else {
				tx_curr->result = msg->value;
				next_state = RADIO_MSG_MGR_STATE_IDLE;
			}
		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state >= 50) {
			next_state = RADIO_MSG_MGR_STATE_RESET;
		}

		break;
	case RADIO_MSG_MGR_STATE_TX_ACK:
		if (msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA) {
			if (mgr->rx_led_on)
				mgr->rx_led_on();

			struct radio_packet_header *h =
					(struct radio_packet_header *) &rx_buff[0];
			if (h->length > 61)
				break;

			struct radio_packet_header *tx_header =
					(struct radio_packet_header *) &tx_curr->data[0];
			if (h->target == mgr->local_address && h->seq == tx_header->seq
					&& (h->flags & 0x80) > 0) {
					tx_curr->result = 1;

				next_state = RADIO_MSG_MGR_STATE_IDLE;
				break;
			}

		} else if (msg->type == NOSYS_TIMER_MSG && mgr->time_in_state >= 100) {
			if (tx_curr->resends < 5) {
				tx_curr->resends++;
			} else {
				next_state = RADIO_MSG_MGR_STATE_IDLE;
				tx_curr->result = 0;
				break;
			}
			if(mgr->tx_led_off)
				mgr->tx_led_off();
			next_state = RADIO_MSG_MGR_STATE_TX;
		}
		break;
	case RADIO_MSG_MGR_STATE_LBT:
		if (msg->type == NOSYS_TIMER_MSG) {
			radio_link_status(&status);
			if (mgr->time_in_state > 10) {
				if (status.rssi <= RSSI_OK) {
					next_state = RADIO_MSG_MGR_STATE_TX;
				} else {
					next_state = RADIO_MSG_MGR_STATE_IDLE;
				}
			}
		} else if (msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA) {
			next_state = RADIO_MSG_MGR_STATE_RX;
		}
		break;
	default:
		next_state = RADIO_MSG_MGR_STATE_RESET;
	}
	return next_state;

}
void radio_msg_mgr_fn(void) {
	struct radio_msg_mgr *mgr = (struct radio_msg_mgr *) self->user_data;
	struct nosys_msg *msg = nosys_msg_get();
	u8_t skip_state = 0;
	if (msg->type == NOSYS_TIMER_MSG) {

		if(mgr->time_in_state % 500 == 0){
//			dbg_printf("Tick %d %x \n",mgr->time_in_state, mgr->state);
		}
		if (mgr->time_in_state < 0xFFFFFFFF) {
			mgr->time_in_state++;
		}
//		if (mgr->time_in_state > 100
//				&& (mgr->state != RADIO_MSG_MGR_STATE_IDLE
//						&& mgr->state != RADIO_MSG_MGR_STATE_RESET)) {
//			tx_curr->result = 0;
//			mgr->state = RADIO_MSG_MGR_STATE_RESET;
//			mgr->time_in_state = 0;
//			msg->type = NOSYS_MSG_STATE;
//		}

	}else if(msg->type == NOSYS_MSG_RADIO_RECIEVED_DATA){
		if(mgr->state == RADIO_MSG_MGR_STATE_RESET){
			skip_state = 1;
		}else if(mgr->state == RADIO_MSG_MGR_STATE_TX_DONE){
			skip_state = 1;
			nosys_msg_postpone(mgr->inq, msg);
		}
	}else if(mgr->state != RADIO_MSG_MGR_STATE_RESET && msg->type == NOSYS_MSG_RADIO_RESET_DONE){
		mgr->state = RADIO_MSG_MGR_STATE_IDLE;
		msg->type = NOSYS_MSG_STATE;
	}

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

