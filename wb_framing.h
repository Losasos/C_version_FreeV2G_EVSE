#ifndef WB_FRAMING_H
#define WB_FRAMING_H

#include "./npcap_lib.h"
#include <time.h>

#define FRAMING_HEADER_SIZE 18
#define FRAMING_HEAD_SIZE 4
#define FRAMING_DATA_SIZE_LENGTH 2
#define DEFAULT_REQUEST_TIMEOUT 100 // millisec

void enable_debug();

typedef struct Response
{
	struct Response *next;
	struct Response *prev;
	uint8_t *payload;
	uint16_t payload_read;
	uint16_t payload_len;
	uint8_t mod_id;
	uint8_t sub_id;
	uint8_t req_id;
	uint8_t checksum;
	int8_t error;
} Response;

typedef struct Framing
{
	pcap_t *pcap;
	size_t req_timeout; // millisec
	struct Response *response_buffer;
	uint8_t *dst;
	uint8_t *src;
	int8_t error;
} Framing;

typedef struct Payload
{
	uint8_t *data;
	unsigned int len;
} Payload;

void _printPayload(uint8_t *payload, int len);
void print_response(struct Response *response);
size_t big_endian_to_int(uint8_t *dest, uint8_t length);
void int_to_big_endian(uint8_t *dest, size_t integer, uint8_t length);
void _valueToExponential(uint8_t *dest, float value);

struct Payload* create_payload(uint8_t *array, size_t length);
struct Payload* create_payload_empty(size_t length);
struct Payload* create_payload_t(size_t value, uint8_t length);
struct Payload* create_payload8_t(uint8_t value);
struct Payload* create_payload16_t(uint16_t value);

void add_int_to_iterator(uint8_t **iterator, size_t integer, uint8_t size);
void add_array_to_iterator(uint8_t **iterator, uint8_t *array, size_t length);
void add_exponential_to_iterator(uint8_t **iterator, float value);

void free_payload(struct Payload **payload);
void free_response(struct Response **response);
void free_framing(struct Framing **framing);

void response_buffer_prepend(struct Framing *framing, struct Response *node);
void extract_from_response_buffer(struct Framing *framing, struct Response *node);
void clear_response_buffer(struct Response **head);

uint8_t calc_checksum(uint8_t data[], uint16_t size);
uint16_t build_framing_protocol_payload(uint8_t **payload, uint8_t mod_id, uint8_t sub_id, uint8_t data[], uint16_t data_size);
unsigned int wb_build_frame(uint8_t **frame, uint8_t dst[], uint8_t src[], uint16_t eth_type, uint8_t payload[], unsigned int payload_len);
void unpack_framing_response(struct pcap_pkthdr *pkt_header, const u_char *pkt_data, struct Response *response);

int8_t is_response_match_filter(struct Response *response, uint8_t mod_id, uint8_t sub_id_fisrt, uint8_t sub_id_last, uint8_t req_id);
struct Response *_sendReceive(struct Framing *framing, uint8_t mod_id, uint8_t sub_id, struct Payload *framing_payload);
struct Response *_sendReceiveAck(struct Framing *framing, uint8_t mod_id, uint8_t sub_id, struct Payload *framing_payload);
uint8_t _sendReceiveAck_responseless(struct Framing *framing, uint8_t mod_id, uint8_t sub_id, struct Payload *framing_payload);
struct Response *_receive(struct Framing *framing, uint8_t mod_id, uint8_t sub_id_fisrt, uint8_t sub_id_last, uint8_t req_id, size_t timeout);

#endif
