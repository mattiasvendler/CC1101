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
#define CMD_TEMPERATURE_MSG			0x07
#define CMD_TEMPERATURE_SAMPLE_RATE 0x08
#define CMD_NODE_POWER_UP			0x09

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

typedef struct radio_msg_temperature{
	struct radio_packet_header header;
	u16_t value;
}__attribute__((packed)) radio_msg_temperature_t;

typedef struct radio_msg_node_power_up {
	struct radio_packet_header header;
}__attribute__((packed)) radio_msg_node_power_up_t;

void radio_msg_hello_init(u8_t *buf,u32_t my_address);
void radio_msg_login_response_init(u8_t *buf, u32_t target,u32_t my_address);
void radio_msg_periodic_status_init(u8_t *buf,u32_t target, u32_t my_address,u16_t value);
void radio_msg_temperature_status_init(u8_t *buf, u32_t target, u32_t my_address, u16_t value);
void radio_msg_node_power_up_init(u8_t *buf, u32_t target, u32_t my_address);
#endif /* SRC_RADIO_RADIO_MSG_H_ */
