/*
 * radio_msg_parser.c
 *
 *  Created on: Mar 11, 2016
 *      Author: mattias
 */
#include <radio_msg_parser.h>
#include <radio_msg.h>
#include <util.h>
#include <types.h>
void radio_msg_parse(u8_t *outbuf,u8_t **inbuf,u8_t len){
	if(len >= sizeof(struct radio_packet_header)){
		struct radio_packet_header *h = (struct radio_packet_header *)outbuf;
		h->source = read32(inbuf);
		h->target = read32(inbuf);
		h->seq = read16(inbuf);
		h->crc =  read8(inbuf);
		h->length = read8(inbuf);
		h->flags = read8(inbuf);
		h->msg_type = read8(inbuf);
	}
}
