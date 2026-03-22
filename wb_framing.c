////////////////////////////////
// This file contains functions for building asending and reciving ethernet pakages
// according to Whitebeet framing protokol

#include "./wb_framing.h"

// struct constructors
// ===================
struct Payload* create_payload(uint8_t *array, size_t length)
{
	struct Payload *payload = (struct Payload *)malloc(sizeof(struct Payload));
	payload->len = length;
	payload->data = malloc(length);
	memcpy(payload->data, array, length);
	return payload;
}

struct Payload* create_payload_empty(size_t length)
{
	struct Payload *payload = (struct Payload *)malloc(sizeof(struct Payload));
	payload->len = length;
	payload->data = malloc(length);
	return payload;
}

struct Payload* create_payload_t(size_t value, uint8_t length)
{
	struct Payload *payload = (struct Payload *)malloc(sizeof(struct Payload));
	payload->len = length;
	payload->data = malloc(length);
	int_to_big_endian(payload->data, value, length);
	return payload;
}

struct Payload* create_payload8_t(uint8_t value)
{
	struct Payload *payload = (struct Payload *)malloc(sizeof(struct Payload));
	payload->len = 1;
	payload->data = malloc(1);
	*(payload->data) = value;
	return payload;
}

struct Payload* create_payload16_t(uint16_t value)
{
	struct Payload *payload = (struct Payload *)malloc(sizeof(struct Payload));
	payload->len = 2;
	payload->data = malloc(2);
	*(payload->data) = (uint8_t)(value >> 8);
	*(payload->data+1) = (uint8_t)value;
	return payload;
}

struct Response *create_response()
{
	struct Response *response = (struct Response *)malloc(sizeof(struct Response));
	response->next = NULL;
	response->prev = NULL;
	response->payload = NULL;
	response->payload_read = 0;
	response->payload_len = 0;
	response->error = 0;
	return response;
}

struct Framing *create_framing()
{
	struct Framing *framing = (struct Framing *)malloc(sizeof(struct Framing));
	framing->pcap = NULL;
	framing->dst = NULL;
	framing->src = NULL;
	framing->req_timeout = DEFAULT_REQUEST_TIMEOUT;
	framing->response_buffer = NULL;
	framing->error = 0;
	return framing;
}

// struct deconstructors
// =====================
void free_payload(struct Payload **payload)
{
	free((*payload)->data);
	free(*payload);
	payload = NULL;
}

void free_response(struct Response **response)
{
	free((*response)->payload);
	free(*response);
	response = NULL;
}

void free_framing(struct Framing **framing)
{
	free((*framing)->pcap); //TODO is it memory safe to free pcap_p type??
	free((*framing)->dst);
	free((*framing)->src);
	clear_response_buffer(&(*framing)->response_buffer);
	free(*framing);
	framing = NULL;
}

// =====================
void _printPayload(uint8_t *payload, int len)
{
	printf("Length of payload: %d \r\n", len);
	printf("Payload:");
	hex_print(payload, len);
	printf("\r\n");
}

size_t big_endian_to_int(uint8_t *dest, uint8_t length)
{	
	size_t integer = 0;
	for(int i=0; i<length; i++)
	{
		integer = integer << 8;
		integer |= (size_t)*dest;
		dest++;
	}
	return integer;
}

void int_to_big_endian(uint8_t *dest, size_t integer, uint8_t length)
{
	uint8_t *adr = (uint8_t *)&integer;
	for(int i=length-1; i>=0; i--)
	{
		dest[i] = *adr;
		adr++;
	}
}

void _valueToExponential(uint8_t *dest, float value)
{
	float tmp = abs(value);
	int16_t base = 0;
    int8_t exponent = 0;
	
	while(tmp > INT16_MAX)
	{
		tmp /= 10;
		exponent++;
	}
	base = (int16_t)tmp;
	
	tmp *= 10;
	while(tmp != 0 && tmp < INT16_MAX)
	{
		base = (int16_t)tmp;
		exponent--;
		tmp *= 10;
	}
	
	if(value < 0)
	{
		// sign change
		base = ~base + 1;
	}
	
	*dest = base >> 8;
	*(++dest) = (uint8_t)base;
	*(++dest) = exponent;
    return;
}

void add_int_to_iterator(uint8_t **iterator, size_t integer, uint8_t size)
{
	int_to_big_endian(*iterator, integer, size);
	*iterator += size;
}

void add_array_to_iterator(uint8_t **iterator, uint8_t *array, size_t length)
{
	memcpy(*iterator, array, length);
	*iterator += length;
}

void add_exponential_to_iterator(uint8_t **iterator, float value)
{
	_valueToExponential(*iterator, value);
	*iterator +=3;
}

// =====================
// according 7stx referece
uint8_t calc_checksum(uint8_t data[], uint16_t size)
{
	uint32_t checksum = 0;
	uint16_t i;
	
	for(i = 0; i < size; i++)
	{
		checksum += data[i];
	}
	
	checksum = (checksum & 0xFFFF) + (checksum >> 16);
	checksum = (checksum & 0xFF) + (checksum >> 8);
	checksum = (checksum & 0xFF) + (checksum >> 8);
	
	if(checksum != 0xFF)
	{
		checksum = ~checksum;
	}
	
	return (uint8_t)checksum;
}

// Builds payload of WB framing protocol
uint16_t build_framing_protocol_payload(uint8_t **payload, uint8_t mod_id, uint8_t sub_id, uint8_t data[], uint16_t data_size)
{
	*payload = malloc(MAX_ETH_FRAME_SIZE);
	uint8_t *iterator = *payload;
	uint16_t payload_len;
	uint8_t checksum;
	
	// frame sequence counter
	static uint8_t req_id = 1;
	
	*iterator = 0xC0; // framing protocol payload start
	*(++iterator) = mod_id;
	*(++iterator) = sub_id;
	*(++iterator) = req_id;
	
	*(++iterator) = data_size >> 8;
	*(++iterator) = data_size;
	
	if(data_size > 0)
	{
		memcpy(++iterator, data, data_size);
		iterator += data_size-1;
	}
	
	*(++iterator) = 0x00; // checksum byte
	*(++iterator) = 0xC1; // framing protocol payload end
	payload_len = ++iterator - *payload;
	
	checksum = calc_checksum(*payload, payload_len);
	*(iterator-2) = checksum;
	
	req_id = (req_id + 1)%255;
	return payload_len;
}

// Builds WB frame
unsigned int wb_build_frame(uint8_t **frame, uint8_t dst[], uint8_t src[], uint16_t eth_type, uint8_t payload[], unsigned int payload_len)
{
	*frame = malloc(MAX_FRAME_SIZE);
	uint8_t *iterator = *frame;
	uint32_t frame_size;
	
	memcpy(iterator, dst, MAC_SIZE);
	iterator += MAC_SIZE;
	memcpy(iterator, src, MAC_SIZE);
	iterator += MAC_SIZE;
	
	*iterator = eth_type >> 8;
	*(++iterator) = eth_type;
	
	// WB frame header
	*(++iterator) = 0x00;
	*(++iterator) = 0x04;
	
	*(++iterator) = payload_len >> 8;
	*(++iterator) = payload_len;
	++iterator;
	
	memcpy(iterator, payload, payload_len);
	iterator += payload_len;
	frame_size = iterator - *frame;
	
	return frame_size;
}

// Converts recieved data into struct
void unpack_framing_response(struct pcap_pkthdr *pkt_header, const u_char *pkt_data, struct Response *response)
{
	response->payload_read = 0;
	pkt_data += FRAMING_HEADER_SIZE;
	response->mod_id = *(++pkt_data);
	response->sub_id = *(++pkt_data);
	response->req_id = *(++pkt_data);
	response->payload_len = ((int16_t) *(++pkt_data)) <<1 | *(++pkt_data);
	
	if(response->payload != NULL)
	{
		free(response->payload);
	}
	response->payload = malloc(response->payload_len);
	memcpy(response->payload, ++pkt_data, response->payload_len);
	response->checksum = *(pkt_data + response->payload_len);
}

void print_response(struct Response *response)
{
	printf("- Response - \r\n");
	printf("mod id: %02X \r\n", response->mod_id);
	printf("sub id: %02X \r\n", response->sub_id);
	printf("req id: %02X \r\n", response->req_id);
	printf("payload len: %d \r\n", response->payload_len);
	printf("payload: ", response->payload_len);
	hex_print(response->payload, response->payload_len);
	printf("\r\n");
	printf("checksum: %02X \r\n", response->checksum);
}

///////////////////////////////
// BUFFERING FUNCTIONS (for incoming messages)
///////////////////////////////
void response_buffer_append(struct Framing *framing, struct Response *node)
{
	if(framing->response_buffer == NULL)
	{
		framing->response_buffer = node;
		node->prev = NULL;
		node->next = NULL;
		return;
	}
	
	struct Response *tmp = framing->response_buffer;
	while(tmp->next != NULL)
	{
		tmp = tmp->next;
	}
	tmp->next = node;
	node->prev = tmp;
	node->next = NULL;
}

void response_buffer_prepend(struct Framing *framing, struct Response *node)
{
	node->next = framing->response_buffer;
	node->prev = NULL;
	if(framing->response_buffer != NULL)
	{
		framing->response_buffer->prev = node;
	}
	framing->response_buffer = node;
}

// removes node from the list
void extract_from_response_buffer(struct Framing *framing, struct Response *node)
{
	if(node->next != NULL)
	{
		node->next->prev = node->prev;
	}
	
	if(node->prev != NULL)
	{
		node->prev->next = node->next;
	}
	else
	{
		framing->response_buffer = node->next;
	}
}

void clear_response_buffer(struct Response **head)
{
	if((*head)->next != NULL)
	{
		clear_response_buffer(&(*head)->next);
	}
	free_response(head);
}

////////////////////////////////////////
///////////// SEND/RECIEVE /////////////
////////////////////////////////////////

struct Response *_sendReceive(Framing *framing, uint8_t mod_id, uint8_t sub_id, struct Payload *framing_payload)
{
	uint8_t *payload;
	uint8_t *frame;
	struct pcap_pkthdr *pkt_header;
	uint8_t *pkt_data;
	struct Response *response;
	uint16_t payload_len;
	uint16_t frame_size;
	uint8_t response_status;
	clock_t end_time;
	int recive_status;
	
	if(framing_payload == NULL)
	{
		payload_len = build_framing_protocol_payload(&payload, mod_id, sub_id, NULL, 0);
	}
	else
	{
		payload_len = build_framing_protocol_payload(&payload, mod_id, sub_id, framing_payload->data, framing_payload->len);
	}
	frame_size = wb_build_frame(&frame, framing->dst, framing->src, ETH_TYPE, payload, payload_len);
	free(payload);
	pcap_sendpacket(framing->pcap, (byte*)frame, frame_size);
	free(frame);
	
	end_time = framing->req_timeout + clock();
	
	do
	{
		response = NULL;
		response_status = pcap_next_ex(framing->pcap, &pkt_header, (const u_char **)&pkt_data);
		recive_status = pcap_handle_status(response_status, framing->pcap);
		
		if(recive_status == 0)
		{
			continue;
		}
		else if(recive_status == -1)
		{
			framing->error = 1;
			printf("framing error\n");
			return NULL;
		}

		response = create_response();
		response->payload = NULL;
		unpack_framing_response(pkt_header, pkt_data, response);
		
		if(response->req_id == 0xFF)
		{
			response_buffer_append(framing, response);
			response = NULL;
			continue;
		}
		
		if(response->mod_id == 0xFF)
		{
			printf("Framing protocol error: %02X \r\n", response->sub_id);
			break;
		}
		
		if(response->sub_id != sub_id)
		{
			printf("Response from mod ID: %02X \r\nwith unexpected sub ID: %02X \r\n", response->mod_id, response->sub_id);	
			break;
		}
		
		if(response->payload_len == 1 && response->payload[0] == 1)
		{
			printf("status: trying to recive again \r\n");
			free(response);
			response = NULL;
			continue;
		}
		return response;
	}
	while(clock() < end_time);
	return response;
}

struct Response *_sendReceiveAck(struct Framing *framing, uint8_t mod_id, uint8_t sub_id, struct Payload *framing_payload)
{
	struct Response *response = _sendReceive(framing, mod_id, sub_id, framing_payload);
	
	if(response == NULL)
    {
		return NULL;
	}
	
    if(response->payload_len == 0)
    {
		printf("Module did not accept command, return code: None \r\n");
		free_response(&response);
		return NULL;
	}
	
    if(response->payload[0] != 0)
    {
    	printf("Module did not accept command, return code: 0x%02X \r\n", response->payload[0]);
		free_response(&response);
    	return NULL;
	}
	
	return response;
}

uint8_t _sendReceiveAck_responseless(struct Framing *framing, uint8_t mod_id, uint8_t sub_id, struct Payload *framing_payload)
{
	struct Response *response = _sendReceiveAck(framing, mod_id, sub_id, framing_payload);
	if(response == NULL)
    {
		return 0;
	}
	free_response(&response);
	return 1;
}

struct Response *_receive(struct Framing *framing, uint8_t mod_id, uint8_t sub_id_fisrt, uint8_t sub_id_last, uint8_t req_id, size_t timeout)
{
	struct Response *response;
	struct pcap_pkthdr *pkt_header;
	uint8_t *pkt_data;
	uint8_t response_status;
	struct Response *iterator = framing->response_buffer;
	clock_t end_time = timeout + clock();
	int recive_status;
	
	// check for status message in response buffer
	while(iterator != NULL)
	{
		if(is_response_match_filter(iterator, mod_id, sub_id_fisrt, sub_id_last, req_id))
		{
			extract_from_response_buffer(framing, iterator);
			return iterator;
		}
		iterator = iterator->next;
	}
	
	// read buffer
	do
	{
		response_status = pcap_next_ex(framing->pcap, &pkt_header, (const u_char **)&pkt_data);
		recive_status = pcap_handle_status(response_status, framing->pcap);
		
		if(recive_status == 0)
		{
			continue;
		}
		else if(recive_status == -1)
		{
			framing->error = 1;
			return NULL;
		}
		
		response = create_response();
		unpack_framing_response(pkt_header, pkt_data, response);
		
		// filtering
		if(is_response_match_filter(response, mod_id, sub_id_fisrt, sub_id_last, req_id))
		{
			return response;
		}
		response_buffer_append(framing, response);
	}
	while(clock() < end_time);
	
	return NULL;
}

int8_t is_response_match_filter(struct Response *response, uint8_t mod_id, uint8_t sub_id_fisrt, uint8_t sub_id_last, uint8_t req_id)
{
	if(mod_id != response->mod_id)
	{
		return 0;
	}
	if(response->sub_id < sub_id_fisrt || response->sub_id > sub_id_last)
	{
		return 0;
	}
	if(req_id != response->req_id)
	{
		return 0;
	}
	
	return 1;
}