/*
 * radio_msg.c
 *
 *  Created on: Mar 11, 2016
 *      Author: mattias
 */
#include <radio_msg.h>
#include <string.h>
static u16_t seq = 0;

void radio_msg_hello_init(u8_t *buf,u32_t my_address) {
	radio_msg_hello_t *hello = (radio_msg_hello_t *) buf;
	memset(hello, 0, sizeof(radio_msg_hello_t));
	seq++;
	hello->header.target = htonl(0xFFFFFFFF);
	hello->header.source = htonl(my_address);
	hello->header.seq = htons(seq);
	hello->header.msg_type = RADIO_MSG_HELLO;
	hello->header.length = 0;
	hello->header.flags = (RADIO_MSG_CTRL << 5);
	hello->header.crc = 0;

}
void radio_msg_login_response_init(u8_t *buf, u32_t target, u32_t my_address) {
	radio_msg_login_response_t *login_response =
			(radio_msg_login_response_t *) buf;
	memset(login_response, 0, sizeof(radio_msg_login_response_t));
	seq++;
	login_response->header.target = htonl(target);
	login_response->header.source = htonl(my_address);
	login_response->header.seq = htons(seq);
	login_response->header.msg_type = CMD_LOGIN_RESPONSE_MSG;
	login_response->header.length = 0;
	login_response->header.flags = (RADIO_MSG_CTRL << 5);
	login_response->header.flags |= 0x10;
	login_response->header.crc = 0;

}
void radio_msg_periodic_status_init(u8_t *buf, u32_t target, u32_t my_address, u16_t value) {
	radio_msg_periodic_status_t *periodic_status =
			(radio_msg_periodic_status_t *) buf;
	memset(periodic_status, 0, sizeof(radio_msg_periodic_status_t));
	seq++;
	periodic_status->header.target = htonl(target);
	periodic_status->header.source = htonl(my_address);
	periodic_status->header.seq = htons(seq);
	periodic_status->header.msg_type = CMD_PERIODIC_STATUS;
	periodic_status->header.length = sizeof(radio_msg_periodic_status_t)- sizeof(struct radio_packet_header);
	periodic_status->header.flags = (RADIO_MSG_CTRL << 5);
	periodic_status->header.flags |= 0x10;;
	periodic_status->header.crc = 0;
	periodic_status->value = htons(value);

}

