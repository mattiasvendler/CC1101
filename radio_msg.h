/*
 * radio_msg.h
 *
 *  Created on: Mar 11, 2016
 *      Author: mattias
 */

#ifndef SRC_RADIO_RADIO_MSG_H_
#define SRC_RADIO_RADIO_MSG_H_
#include <types.h>
#define RADIO_MSG_CTRL 0x01
#define RADIO_MSG_QUERY 0x02
#define RADIO_MSG_PROPERTY 0x03
#define RADIO_REQIRE_ACK 0x10

#define RADIO_MSG_HELLO 			0x01
#define CMD_LOGIN_REQUEST_MSG 		0x02
#define CMD_LOGIN_RESPONSE_MSG  	0x03
#define CMD_FLOORHEAT_STATE_MSG  	0x04
#define CMD_PING_PONG_MSG  			0x05
#define CMD_PERIODIC_STATUS			0x06

#define htons(A) ((((A) & 0xff00) >> 8) | (((A) & 0x00ff) << 8))

#define htonl(A) ((((A) & 0xff000000) >> 24) | \
(((A) & 0x00ff0000) >> 8) | \
(((A) & 0x0000ff00) << 8) | \
(((A) & 0x000000ff) << 24))


struct radio_packet_header {
	u32_t source;
	u32_t target;
	u16_t seq;
	u8_t crc;
	u8_t length;
	u8_t flags;
	u8_t msg_type;
}__attribute__((packed));

typedef struct radio_msg_hello {
	struct radio_packet_header header;
}__attribute__((packed)) radio_msg_hello_t;

typedef struct radio_msg_login_response {
	struct radio_packet_header header;
}__attribute__((packed)) radio_msg_login_response_t;

typedef struct radio_msg_periodic_status{
	struct radio_packet_header header;
	u16_t value;
}__attribute__((packed)) radio_msg_periodic_status_t;

void radio_msg_hello_init(u8_t *buf);
void radio_msg_login_response_init(u8_t *buf, u32_t target);
void radio_msg_periodic_status_init(u8_t *buf,u32_t target, u16_t value);

#endif /* SRC_RADIO_RADIO_MSG_H_ */
