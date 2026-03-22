////////////////////////////////
// This file contains some functions for sending and reciving messages from Whitebeet
// Each funcion represents a sertan message to a Whitebeet
// more information about messages in the Whitebeet User Manual from 7Stax
// Only EVSE side messages were tested more info on what functions are tested in the header file

#include "./whitebeet.h"

// System sub IDs
#define sys_mod_id 0x10
#define sys_sub_get_firmware_version 0x41

// Network configuration IDs
#define netconf_sub_id 0x05
#define netconf_set_port_mirror_state 0x55

// SLAC module IDs
#define slac_mod_id 0x28
#define slac_sub_start 0x42
#define slac_sub_stop 0x43
#define slac_sub_match 0x44
#define slac_sub_start_match 0x44
#define slac_sub_set_validation_configuration 0x4B
#define slac_sub_join 0x4D
#define slac_sub_success 0x80
#define slac_sub_failed 0x81
#define slac_sub_join_status 0x84

// CP module IDs
#define cp_mod_id 0x29
#define cp_sub_set_mode 0x40
#define cp_sub_get_mode 0x41
#define cp_sub_start 0x42
#define cp_sub_stop 0x43
#define cp_sub_set_dc 0x44
#define cp_sub_get_dc 0x45
#define cp_sub_set_res 0x46
#define cp_sub_get_res 0x47
#define cp_sub_get_state 0x48
#define cp_sub_nc_state 0x81

// V2G module IDs
#define v2g_mod_id 0x27
#define v2g_sub_set_mode 0x40
#define v2g_sub_get_mode 0x41
#define v2g_sub_start 0x42
#define v2g_sub_stop 0x43

// EV sub IDs
#define v2g_sub_ev_set_configuration 0xA0
#define v2g_sub_ev_get_configuration 0xA1
#define v2g_sub_ev_set_dc_charging_parameters 0xA2
#define v2g_sub_ev_update_dc_charging_parameters 0xA3
#define v2g_sub_ev_get_dc_charging_parameters 0xA4
#define v2g_sub_ev_set_ac_charging_parameters 0xA5
#define v2g_sub_ev_update_ac_charging_parameters 0xA6
#define v2g_sub_ev_get_ac_charging_parameters 0xA7
#define v2g_sub_ev_set_charging_profile 0xA8
#define v2g_sub_ev_start_session 0xA9
#define v2g_sub_ev_start_cable_check 0xAA
#define v2g_sub_ev_start_pre_charging 0xAB
#define v2g_sub_ev_start_charging 0xAC
#define v2g_sub_ev_stop_charging 0xAD
#define v2g_sub_ev_stop_session 0xAE

// EVSE sub IDs
#define v2g_sub_evse_set_configuration 0x60
#define v2g_sub_evse_get_configuration 0x61
#define v2g_sub_evse_set_dc_charging_parameters 0x62
#define v2g_sub_evse_update_dc_charging_parameters 0x63
#define v2g_sub_evse_get_dc_charging_parameters 0x64
#define v2g_sub_evse_set_ac_charging_parameters 0x65
#define v2g_sub_evse_update_ac_charging_parameters 0x66
#define v2g_sub_evse_get_ac_charging_parameters 0x67
#define v2g_sub_evse_set_sdp_config 0x68
#define v2g_sub_evse_get_sdp_config 0x69
#define v2g_sub_evse_start_listen 0x6A
#define v2g_sub_evse_set_authorization_status 0x6B
#define v2g_sub_evse_set_schedules 0x6C
#define v2g_sub_evse_set_cable_check_finished 0x6D
#define v2g_sub_evse_start_charging 0x6E
#define v2g_sub_evse_stop_charging 0x6F
#define v2g_sub_evse_stop_listen 0x70
#define v2g_sub_evse_set_cable_certificate_installation_and_update_response 0x73
#define v2g_sub_evse_set_meter_receipt 0x74
#define v2g_sub_evse_send_notification 0x75
#define v2g_sub_evse_set_session_parameter_timeout 0x76

// Helper function for parsing payload. Reads an integer from the payload.
// If an integer schuld be signed parse recieved return value
size_t payloadReaderReadInt(struct Response *response, size_t num)
{
    size_t value = 0;	
    if(response->payload_read + num > response->payload_len)
    {
		printf("Less payload than expected!\n");
        return -1;
    }
	if(num == 1)
	{
		value = response->payload[response->payload_read];
	}
	else
	{
		value = big_endian_to_int(response->payload + response->payload_read, num);
	}
	response->payload_read += num;
    return value;
}

// Helper function for parsing payload. Reads an exponential from the payload.
float payloadReaderReadExponential(struct Response *response)
{
	if(response->payload_read + 3 > response->payload_len)
    {
		printf("Less payload than expected!\n");
        return 0;
    }
	
	float number = (float)payloadReaderReadInt(response, 2);
	int8_t exp = *(response->payload + response->payload_read);
	response->payload_read++;
	
	int i = 0;
	for(i; i<exp; i++)
	{
		number *= 10;
	}
	for(i; i>exp; i--)
	{
		number /= 10;
	}
	
    return number;
}

// Helper function for parsing payload. Reads a number of bytes from the payload.
int8_t payloadReaderReadBytes(void *dest, struct Response *response, size_t num)
{
    if(response->payload_read + num > response->payload_len)
    {
		printf("Less payload than expected!\n");
        response->error = 1;
        return -1;
    }
	
	memcpy(dest, response->payload + response->payload_read, num);
	response->payload_read += num;
    return 0;
}

// Helper function for parsing payload. Finalizes the reading. Checks if payload was read;
// completely. Raises a ValueError otherwise.
int8_t payloadReaderFinalize(struct Response *response)
{
    if(response->payload_read != response->payload_len)
    {
        printf("More payload to be read is available! (read: %d, length: %d)\n", response->payload_read, response->payload_len);
        return -1;
    }
	return 0;
}

void payloadReaderInitialize(struct Response *response)
{
	response->payload_read = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// end of utility functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Retrives the firmware version in the form x.x.x
int systemGetVersion(struct Framing *framing, char **version)
{
    struct Response *response = _sendReceiveAck(framing, sys_mod_id, sys_sub_get_firmware_version, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return -1;
	}
	
    uint8_t version_length = payloadReaderReadInt(response, 2);
	*version = (char *)malloc(version_length+1);
	payloadReaderReadBytes(*version, response, version_length);
	*version[version_length] = 0;
	free_response(&response);
    return version_length;
}

// Sets the mode of the control pilot service.
// 0: EV, 1: EVSE
int8_t controlPilotSetMode(struct Framing *framing, uint8_t mode)
{
	if(mode != 0 && mode != 1)
	{
		printf("CP mode can be either 0 or 1 \r\n");
		framing->error = 1;
		return -1;
	}
	
	struct Payload *payload = create_payload8_t(1);
	int status = _sendReceiveAck_responseless(framing, cp_mod_id, cp_sub_set_mode, payload);
	free_payload(&payload);
	return status;
}

// Sets the mode of the control pilot service.
// Returns: 0: EV, 1: EVSE, 255: Mode was not yet set
int16_t controlPilotGetMode(struct Framing* framing)
{
	struct Response *response = _sendReceiveAck(framing, cp_mod_id, cp_sub_get_mode, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return -1;
	}
	
	uint16_t len = response->payload_len;
	uint8_t value = response->payload[1];
	free_response(&response);
	
	if(len != 2)
    {
		printf("Module returned malformed message with length %d \r\n", len);
		framing->error = 1;
		return -1;
    }
	
    if(value != 0 && value != 1 && value != 255)
    {
		printf("Module returned invalid mode %d \r\n", value);
		framing->error = 1;
		return -1;
    }
	
	return value;
}

// Starts the control pilot service.
int8_t controlPilotStart(struct Framing* framing)
{
	return _sendReceiveAck_responseless(framing, cp_mod_id, cp_sub_start, NULL);
}

// Stops the control pilot service.
uint8_t controlPilotStop(struct Framing* framing)
{
	struct Response *response = _sendReceive(framing, cp_mod_id, cp_sub_stop, NULL);
	uint8_t value = response->payload[1];
	free_response(&response);
	
	if(value != 0 && value !=  5)
	{
		printf("CP module did not accept our stop command \r\n");
		framing->error = 1;
		return -1;
	}
	
	return value;
}

int8_t controlPilotSetDutyCycle(struct Framing *framing, float duty_cycle)
{
    if(duty_cycle < 0 || duty_cycle > 100)
    {
        printf("Duty cycle parameter needs to be between 0 and 100\n");
        framing->error = 1;
		return -1;
    }
	
	uint16_t duty_cycle_permill = duty_cycle * 10;

	struct Payload *payload = create_payload16_t(duty_cycle_permill);
	int status = _sendReceiveAck_responseless(framing, cp_mod_id, cp_sub_set_dc, payload);
	return status;
}

// Returns the currently configured duty cycle
float controlPilotGetDutyCycle(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, cp_mod_id, cp_sub_get_dc, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return -1;
	}
	
    if(response->payload_len != 3)
    {
        printf("Module returned malformed message with length %d\n", response->payload_len);
        framing->error = 1;
		free_response(&response);
        return -1;
    }
	
	float duty_cycle = (float)big_endian_to_int(response->payload+1, 2) / 10;
	free_response(&response);
	
	if(duty_cycle < 0 || duty_cycle > 100)
	{
		printf("Module returned invalid duty cycle %d\n", duty_cycle);
		framing->error = 1;
		return -1;
	}
	
	return duty_cycle;
}

// Returns the state of the resistor value
int8_t controlPilotGetResistorValue(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, cp_mod_id, cp_sub_get_res, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return -1;
	}
	
	uint16_t payload_len = response->payload_len;
	uint8_t value = response->payload[1];
	free_response(&response);
	
    if(payload_len != 1)
    {
        printf("Module returned malformed message with length %d\n", payload_len);
        framing->error = 1;
        return -1;
    }
	
    if(value < 0 || value > 4)
    {
        printf("Module returned invalid state %d\n", value);
        framing->error = 1;
        return -1;
    }
	
	return value;
}

// Returns the state of the resistor value
int8_t controlPilotSetResistorValue(struct Framing *framing, uint8_t value)
{
    if(value < 0 || value > 1)
    {
        printf("Resistor value needs to be 0 or 1\n");
        return -1;
    }
	
	struct Payload *payload = create_payload8_t(value);
    struct Response *response = _sendReceiveAck(framing, cp_mod_id, cp_sub_set_res, payload);
	free_payload(&payload);
	
	if(response == NULL)
	{
		framing->error = 1;
		return -1;
	}
	
	uint16_t payload_len = response->payload_len;
	uint8_t response_value = response->payload[1];
	free_response(&response);
	
    if(payload_len != 1)
    {
        printf("Module returned malformed message with length %d\n", payload_len);
        framing->error = 1;
        return -1;
    }
	
    if(response_value < 0 || response_value > 4)
    {
        printf("Module returned invalid state %d\n", response_value);
        framing->error = 1;
        return -1;
    }
	
	return response_value;
}

// Returns the state on the CP
// 0: state A, 1: state B, 2: state C, 3: state D, 4: state E, 5: state F, 6: Unknown
int8_t controlPilotGetState(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, cp_mod_id, cp_sub_get_state, NULL);
	if(response == NULL)
	{
		return -1;
	}
	uint16_t payload_len = response->payload_len;
	uint8_t value = response->payload[1];
	free_response(&response);
	
    if(payload_len != 2)
    {
        printf("Module returned malformed message with length %d\n", payload_len);
        framing->error = 1;
        return -1;
    }
    if(value < 0 || value > 6)
    {
        printf("Module returned invalid state %d\n", value);
        framing->error = 1;
        return -1;
    }
	
	return value;
}

// Starts the SLAC service.
int8_t slacStart(struct Framing *framing, uint8_t mode_in)
{
    if(mode_in < 0 || mode_in > 1)
    {
        printf("Mode parameter needs to be 0 (EV) or 1 (EVSE)\n");
        framing->error = 1;
		return -1;
    }
	
	struct Payload *payload = create_payload8_t(mode_in);
	int status = _sendReceiveAck_responseless(framing, slac_mod_id, slac_sub_start, payload);
	free_payload(&payload);
	return status;
}

// Stops the SLAC service.
int8_t slacStop(struct Framing *framing)
{
    struct Response *response =_sendReceive(framing, slac_mod_id, slac_sub_stop, NULL);
	
    if(response->payload[0] < 0 || response->payload[0] < 0x10)
    {
        printf("SLAC module did not accept our stop command\n");
        framing->error = 1;
		free_response(&response);
		return -1;
    }
	
	free_response(&response);
	return 0;
}

// Starts the SLAC matching process on EV side
void slacStartMatching(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, slac_mod_id, slac_sub_match, NULL);
}

// Waits for SLAC success or failed message for matching process
int8_t slacMatched(struct Framing *framing)
{
    clock_t time_start = clock();
    struct Response *response = _receive(framing, slac_mod_id, slac_sub_success, slac_sub_failed, 0xFF, 60000);
	if(response == NULL)
	{
		printf("No Response\n");
		return -1;
	}
	
	//TODO it is better to get time_end from response
	clock_t time_end = clock();
	uint16_t payload_len = response->payload_len;
	uint8_t response_sub_id = response->sub_id;
	free_response(&response);
	
    if(payload_len != 0)
    {
        printf("Module returned malformed message with length %d\n", payload_len);
        framing->error = 1;
        return -1;
    }
	
    if(response_sub_id == slac_sub_success)
    {
        return 1;
    }
	
	if(time_end - time_start > 49000)
	{
		printf("SLAC matching timed out\n");
		framing->error = 1;
		return -1;
	}
	
	return 0;
}

int8_t networkConfigSetPortMirrorState(struct Framing *framing, uint8_t value)
{
    if(value < 0 || value > 1)
    {
        printf("Value needs to be of type int with value 0 or 1\n");
        framing->error = 1;
		return -1;
    }
	
	struct Payload *payload = create_payload8_t(value);
	_sendReceiveAck_responseless(framing, netconf_sub_id, netconf_set_port_mirror_state, payload);
	free_payload(&payload);
	return 0;
}

// Joins a network
int8_t slacJoinNetwork(struct Framing *framing, uint8_t *nid, uint8_t *nmk)
{
    if(NID_SIZE != 7)
    {
        printf("NID parameter needs to be of length 7 (is %d)\n", NID_SIZE);
        framing->error = 1;
        return -1;
    }

    if(NMK_SIZE != 16)
    {
        printf("NMK parameter needs to be of length 16 (is %d)\n", NMK_SIZE);
        framing->error = 1;
        return -1;
    }
	
	struct Payload *payload = create_payload_empty(NID_SIZE + NMK_SIZE);
	payload->len = NID_SIZE + NMK_SIZE;
	memcpy(payload->data, nid, NID_SIZE);
	memcpy(payload->data+NID_SIZE, nmk, NMK_SIZE);
	
	_sendReceiveAck_responseless(framing, slac_mod_id, slac_sub_join, payload);
	free_payload(&payload);
	return 0;
}

// Waits for SLAC success or failed message for joining process;
int8_t slacJoined(struct Framing *framing)
{
    struct Response *response =_receive(framing, slac_mod_id, slac_sub_join_status, slac_sub_join_status, 0xFF, 30);
	if(response == NULL)
	{
		framing->error = 1;
		return -1;
	}
	
	uint16_t payload_len = response->payload_len;
	uint8_t value = response->payload[0];
	free_response(&response);
	
    if(payload_len != 1)
    {
        printf("Module sent malformed message with length %d\n", payload_len);
        framing->error = 1;
        return -1;
    }
    
	if(value < 0 || value > 1)
    {
        printf("Module sent invalid status %d\n", response->payload[0]);
        framing->error = 1;
        return -1;
    }
    
	if(value == 0)
    {
        return 0;
    }
	
	return 1;
}

// Enables or disables validation;
int8_t slacSetValidationConfiguration(struct Framing *framing, uint8_t configuration)
{
    if(configuration < 0 || configuration > 1)
    {
        printf("Parameter configuration needs to be of type int with value 0 or 1\n");
		framing->error = 1;
		return -1;
    }
	
	struct Payload *payload = create_payload8_t(configuration);
	_sendReceiveAck_responseless(framing, slac_mod_id, slac_sub_set_validation_configuration, payload);
	free_payload(&payload);
	return 0;
}

// 0: EV, 1: EVSE;
int8_t v2gSetMode(struct Framing *framing, uint8_t mode)
{
    if(mode < 0 || mode > 1)
    {
        printf("V2G mode can be either 0 or 1\n");
        framing->error = 1;
        return -1;
    }
	
	
	struct Payload *payload = create_payload8_t(mode);
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_set_mode, payload);
	free_payload(&payload);
	return 0;
}

// Returns the mode of the V2G service.
// Returns: 0: EV, 1: EVSE, 2: Mode was not yet set;
int8_t v2gGetMode(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, v2g_mod_id, v2g_sub_get_mode, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return -1;
	}
	
    uint16_t payload_len = response->payload_len;
    uint8_t payload_data = response->payload[1];

    if(payload_len != 2)
    {
        printf("Module returned malformed message with length %d\n", payload_len);
        framing->error = 1;
        return -1;
    }
	
    if(payload_data < 0 || payload_data > 2)
    {
        printf("Module returned invalid mode %d\n", response->payload[1]);
        framing->error = 1;
        return -1;
    }
	
	return payload_data;
}

// Starts the v2g service.
void v2gStart(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_start, NULL);
}

// Stops the v2g service.
void v2gStop(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_stop, NULL);
}

// Sets the configuration for EV mode;
void v2gEvSetConfiguration(struct Framing *framing, struct EV_config *config)
{
	// "evid", "protocol_count", "protocols", "payment_method_count", "payment_method", "energy_transfer_mode_count", "energy_transfer_mode", "battery_capacity", "battery_capacity"
	struct Payload *payload = create_payload_empty(sizeof(struct EV_config));
	uint8_t *iterator = payload->data;
	
	add_array_to_iterator(&iterator, config->evid, 6);
	add_int_to_iterator(&iterator, config->protocol_count, 1);
	for(int i=0; i < config->protocol_count; i++)
	{
		add_int_to_iterator(&iterator, config->protocol[i], 1);
	}
	
	add_int_to_iterator(&iterator, config->payment_method_count, 1);
	for(int i=0; i < config->payment_method_count; i++)
	{
		add_int_to_iterator(&iterator, config->payment_method[i], 1);
	}
	
	add_int_to_iterator(&iterator, config->energy_transfer_mode_count, 1);
	for(int i=0; i < config->energy_transfer_mode_count; i++)
	{
		add_int_to_iterator(&iterator, config->energy_transfer_mode[i], 1);
	}
	
	add_exponential_to_iterator(&iterator, config->battery_capacity);
	payload->len = iterator - payload->data;
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_set_configuration, payload);
	free_payload(&payload);
}

// Get the configuration of EV mdoe;
// Returns dictionary;
struct EV_config *v2gEvGetConfiguration(struct Framing *framing)
{
    struct EV_config *ev_config = (struct EV_config *) malloc(sizeof(struct EV_config));
    struct Response *response = _sendReceiveAck(framing, v2g_mod_id, v2g_sub_ev_get_configuration, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return NULL;
	}
	
    payloadReaderReadInt(response, 1);
    payloadReaderReadBytes(ev_config->evid, response, 6);
    ev_config->protocol_count = payloadReaderReadInt(response, 1);
    for(int i=0; i < ev_config->protocol_count; i++)
    {
        ev_config->protocol[i] = payloadReaderReadInt(response, 1);
    }
    
    ev_config->payment_method_count = payloadReaderReadInt(response, 1);
    for(int i=0; i < ev_config->payment_method_count; i++)
    {
        ev_config->payment_method[i] = payloadReaderReadInt(response, 1);
    }
    
    ev_config->energy_transfer_mode_count = payloadReaderReadInt(response, 1);
    for(int i=0; i < ev_config->energy_transfer_mode_count; i++)
    {
		ev_config->energy_transfer_mode[i] = payloadReaderReadInt(response, 1);
    }
    
    ev_config->battery_capacity = payloadReaderReadExponential(response);
    
    payloadReaderFinalize(response);
	free_response(&response);
    return ev_config;
}

// Sets the DC charging parameters of the EV;
void v2gSetDCChargingParameters(struct Framing *framing, struct EV_DC_param *params)
{
	// "min_voltage", "min_current", "min_power", "status", "energy_request", "departure_time", "max_voltage", "max_current", "max_power", "soc", "target_voltage", "target_current", "full_soc", "bulk_soc"
	struct Payload *payload = create_payload_empty(sizeof(EV_DC_param));
	uint8_t *iterator = payload->data;
	
	add_exponential_to_iterator(&iterator, params->min_voltage);
	add_exponential_to_iterator(&iterator, params->min_current);
	add_exponential_to_iterator(&iterator, params->min_power);
	add_exponential_to_iterator(&iterator, params->max_voltage);
	add_exponential_to_iterator(&iterator, params->max_current);
	add_exponential_to_iterator(&iterator, params->max_power);
	add_int_to_iterator(&iterator, params->soc, 1);
	add_int_to_iterator(&iterator, params->status, 1);
	add_exponential_to_iterator(&iterator, params->target_voltage);
	add_exponential_to_iterator(&iterator, params->target_current);
	add_int_to_iterator(&iterator, params->full_soc, 1);
	add_int_to_iterator(&iterator, params->bulk_soc, 1);
	add_exponential_to_iterator(&iterator, params->energy_request);
	add_int_to_iterator(&iterator, params->departure_time, 4);
	
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_set_dc_charging_parameters, payload);
	free_payload(&payload);
}

// Updates the DC charging parameters of the EV;
void v2gUpdateDCChargingParameters(struct Framing *framing, struct EV_DC_param *params)
{
	struct Payload *payload = create_payload_empty(sizeof(EV_DC_param));
	uint8_t *iterator = payload->data;
	
	add_exponential_to_iterator(&iterator, params->min_voltage);
	add_exponential_to_iterator(&iterator, params->min_current);
	add_exponential_to_iterator(&iterator, params->min_power);
	add_exponential_to_iterator(&iterator, params->max_voltage);
	add_exponential_to_iterator(&iterator, params->max_current);
	add_exponential_to_iterator(&iterator, params->max_power);
	add_int_to_iterator(&iterator, params->soc, 1);
	add_int_to_iterator(&iterator, params->status, 1);
	add_exponential_to_iterator(&iterator, params->target_voltage);
	add_exponential_to_iterator(&iterator, params->target_current);
	
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_update_dc_charging_parameters, payload);
	free_payload(&payload);
}

// Gets the DC charging parameters;
// Returns dictionary;
struct EV_DC_param *v2gGetDCChargingParameters(struct Framing *framing)
{
	// "min_voltage", "min_current", "min_power", "status","max_voltage", "max_current", "max_power", "soc", "target_voltage", "target_current"
    struct Response *response = _sendReceiveAck(framing, v2g_mod_id, v2g_sub_ev_get_dc_charging_parameters, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return NULL;
	}
	
	struct EV_DC_param *params = (struct EV_DC_param *)malloc(sizeof(struct EV_DC_param));
	
    payloadReaderReadInt(response, 1);
    params->min_voltage = payloadReaderReadExponential(response);
    params->min_current = payloadReaderReadExponential(response);
    params->min_power = payloadReaderReadExponential(response);
    params->max_voltage = payloadReaderReadExponential(response);
    params->max_current = payloadReaderReadExponential(response);
    params->max_power = payloadReaderReadExponential(response);
    params->soc = payloadReaderReadInt(response, 1);
    params->status = payloadReaderReadInt(response, 1);
    params->full_soc = payloadReaderReadInt(response, 1);
    params->bulk_soc = payloadReaderReadInt(response, 1);
    params->energy_request = payloadReaderReadExponential(response);
    params->departure_time = payloadReaderReadInt(response, 4);
    payloadReaderFinalize(response);
	
	free_response(&response);
    return params;
}

// Sets the AC charging parameters of the EV;
void v2gSetACChargingParameters(struct Framing *framing, struct EV_AC_param *parameter)
{
    // "min_voltage", "min_current", "min_power", "energy_request", "departure_time", "max_voltage", "max_current", "max_power"
	
	struct Payload *payload = create_payload_empty(sizeof(struct EV_AC_param));
	uint8_t *iterator = payload->data;
	
	add_exponential_to_iterator(&iterator, parameter->min_voltage);
	add_exponential_to_iterator(&iterator, parameter->min_current);
	add_exponential_to_iterator(&iterator, parameter->min_power);
	add_exponential_to_iterator(&iterator, parameter->max_voltage);
	add_exponential_to_iterator(&iterator, parameter->max_current);
	add_exponential_to_iterator(&iterator, parameter->max_power);
	add_exponential_to_iterator(&iterator, parameter->energy_request);
	add_int_to_iterator(&iterator, parameter->departure_time, 4);
	
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_set_ac_charging_parameters, payload);
	free_payload(&payload);
}

// Updates the AC charging parameters of the EV;
void v2gUpdateACChargingParameters(struct Framing *framing, struct EV_AC_param *parameter)
{
    // "min_current", "min_voltage", "min_power", "max_voltage", "max_current", "max_power"
    struct Payload *payload = create_payload_empty(sizeof(struct EV_AC_param));
	uint8_t *iterator = payload->data;
    
    add_exponential_to_iterator(&iterator, parameter->min_voltage);
    add_exponential_to_iterator(&iterator, parameter->min_current);
    add_exponential_to_iterator(&iterator, parameter->min_power);
    add_exponential_to_iterator(&iterator, parameter->max_voltage);
    add_exponential_to_iterator(&iterator, parameter->max_current);
    add_exponential_to_iterator(&iterator, parameter->max_power);
    
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_update_ac_charging_parameters, payload);
	free_payload(&payload);
}

// Gets the AC charging parameters;
// Returns dictionary;
struct EV_AC_param *v2gACGetChargingParameters(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, v2g_mod_id, v2g_sub_ev_get_dc_charging_parameters, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return NULL;
	}
	
	struct EV_AC_param *params = (struct EV_AC_param *)malloc(sizeof(struct EV_AC_param));
    payloadReaderReadInt(response, 1);
    params->min_voltage = payloadReaderReadExponential(response);
    params->min_current = payloadReaderReadExponential(response);
    params->min_power = payloadReaderReadExponential(response);
    params->max_voltage = payloadReaderReadExponential(response);
    params->max_current = payloadReaderReadExponential(response);
    params->max_power = payloadReaderReadExponential(response);
    params->energy_request = payloadReaderReadExponential(response);
    params->departure_time = payloadReaderReadInt(response, 4);
    payloadReaderFinalize(response);
	
	free_response(&response);
    return params;
}

// Sets the charging profile;
// TODO rebuild struct to array of structs nort arrays inside a struct
void v2gSetChargingProfile(struct Framing *framing, struct Profile *schedule)
{
	struct Payload *payload = create_payload_empty(sizeof(struct Profile));
	uint8_t *iterator = payload->data;
	
	add_int_to_iterator(&iterator, schedule->schedule_tuple_id, 2);
	add_int_to_iterator(&iterator, schedule->charging_profile_entries_count, 1);
    
    for(int i=0; i < schedule->charging_profile_entries_count; i++)
    {
        add_int_to_iterator(&iterator, schedule->start[i], 4);
        add_int_to_iterator(&iterator, schedule->interval[i], 4);
        add_exponential_to_iterator(&iterator, schedule->power[i]);
    }
	
	payload->len = iterator - payload->data;
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_set_charging_profile, payload);
	free_payload(&payload);
}

// Starts a new charging session;
void v2gStartSession(struct Framing *framing)
{
            
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_start_session, NULL);
}

// Starts the cable check after notification Cable Check Ready has been reveived;
void v2gStartCableCheck(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_start_cable_check, NULL);
}

// Starts the pre charging after notification Pre Charging Ready has been reveived;
void v2gStartPreCharging(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_start_pre_charging, NULL);
}

// Starts the  charging after notification Charging Ready has been reveived;
void v2gStartCharging(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_start_charging, NULL);
}

// Stops the charging;
void v2gStopCharging(struct Framing *framing, uint8_t renegotiation)
{
	struct Payload *payload = create_payload8_t(renegotiation);
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_stop_charging, payload);
}

// Stops the currently active charging session after the notification Post Charging Ready has been received.
// When Charging in AC mode the session is stopped auotamically because no post charging needs to be performed.
void v2gStopSession(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_ev_stop_session, NULL);
}

// Parse a session started message.
struct EV_session_started *v2gEvParseSessionStarted(struct Framing *framing, struct Response *response)
{
	struct EV_session_started *message = (struct EV_session_started *)malloc(sizeof(struct EV_session_started));
	
    message->protocol = payloadReaderReadInt(response, 1);
    payloadReaderReadBytes(message->session_id, response, 8);
	
	message->evse_id_len = payloadReaderReadInt(response, 1);
	message->evse_id[message->evse_id_len] = 0;
    payloadReaderReadBytes(message->evse_id, response, message->evse_id_len);
	
    message->payment_method = payloadReaderReadInt(response, 1);
    message->energy_transfer_mode = payloadReaderReadInt(response, 1);
    payloadReaderFinalize(response);
	
    return message;
}

// Parse a DC charge parameters changed message.
struct DC_charge_param *v2gEvParseDCChargeParametersChanged(struct Framing *framing, struct Response *response)
{
	struct DC_charge_param *message = (struct DC_charge_param *)malloc(sizeof(struct DC_charge_param));
	
    message->evse_min_voltage = payloadReaderReadExponential(response);
    message->evse_min_current = payloadReaderReadExponential(response);
    message->evse_min_power = payloadReaderReadExponential(response);
    message->evse_max_voltage = payloadReaderReadExponential(response);
    message->evse_max_current = payloadReaderReadExponential(response);
    message->evse_max_power = payloadReaderReadExponential(response);
    message->evse_present_voltage = payloadReaderReadExponential(response);
    message->evse_present_current = payloadReaderReadExponential(response);
    message->evse_status = payloadReaderReadInt(response, 1);
	
    if(payloadReaderReadInt(response, 1) != 0)
    {
        message->evse_isolation_status = payloadReaderReadInt(response, 1);
    }
	
    message->evse_voltage_limit_achieved = payloadReaderReadInt(response, 1);
    message->evse_current_limit_achieved = payloadReaderReadInt(response, 1);
    message->evse_power_limit_achieved = payloadReaderReadInt(response, 1);
    message->evse_peak_current_ripple = payloadReaderReadExponential(response);
	
    if(payloadReaderReadInt(response, 1) != 0)
    {
        message->evse_current_regulation_tolerance = payloadReaderReadExponential(response);
    }
    if(payloadReaderReadInt(response, 1) != 0)
    {
        message->evse_energy_to_be_delivered = payloadReaderReadExponential(response);
    }
    payloadReaderFinalize(response);
	
    return message;
}

// Parse a AC charge parameters changed message.
struct AC_charge_param *v2gEvParseACChargeParametersChanged(struct Framing *framing, struct Response *response)
{
    struct AC_charge_param *message = (struct AC_charge_param *)malloc(sizeof(struct AC_charge_param));
	
    message->nominal_voltage = payloadReaderReadExponential(response);
    message->max_current = payloadReaderReadExponential(response);
    message->max_current = payloadReaderReadExponential(response);
    message->rcd = payloadReaderReadInt(response, 1);
    payloadReaderFinalize(response);
	
    return message;
}

// Parse a schedule received message.
struct Schedule_profile *v2gEvParseScheduleReceived(struct Framing *framing, struct Response *response)
{
	struct Schedule_profile *message = (struct Schedule_profile *)malloc(sizeof(struct Schedule_profile));
	
    message->tuple_number = payloadReaderReadInt(response, 1);
    message->schedule_tuple_id = payloadReaderReadInt(response, 2);
    message->schedule_entries_count = payloadReaderReadInt(response, 2);
    
    for(int i=0; i<message->schedule_entries_count; i++)
    {
        message->schedule_entries[i].start = payloadReaderReadInt(response, 4);
        message->schedule_entries[i].interval = payloadReaderReadInt(response, 4);
        message->schedule_entries[i].power = payloadReaderReadExponential(response);
    }
    payloadReaderFinalize(response);
    
    return message;
}

// Parse a notification received message.
struct EV_notification *v2gEvParseNotificationReceived(struct Framing *framing, struct Response *response)
{
	struct EV_notification *message = (struct EV_notification *)malloc(sizeof(struct EV_notification));
    message->type = payloadReaderReadInt(response, 1);
    message->max_delay = payloadReaderReadInt(response, 2);
    payloadReaderFinalize(response);
    return message;
}

// Parse a session error message.
int8_t v2gEvParseSessionError(struct Framing *framing, struct Response *response)
{
    uint8_t error_code = payloadReaderReadInt(response, 1);
    payloadReaderFinalize(response);
    return error_code;
}

// Sets the configuration;
void v2gEvseSetConfiguration(struct Framing *framing, struct EVSE_config *config)
{
	// "evse_id_DIN", "evse_id_ISO", "protocol", "payment_method", "certificate_installation_support", "certificate_update_support", "energy_transfer_mode"
	struct Payload *payload = create_payload_empty(sizeof(struct EVSE_config));
	uint8_t *iterator = payload->data;
    int i;
	
	add_int_to_iterator(&iterator, config->evse_id_DIN_len, 1);
	add_array_to_iterator(&iterator, config->evse_id_DIN, config->evse_id_DIN_len);
	add_int_to_iterator(&iterator, config->evse_id_ISO_len, 1);
	add_array_to_iterator(&iterator, config->evse_id_ISO, config->evse_id_ISO_len);
	
	add_int_to_iterator(&iterator, config->protocol_count, 1);
	for(i=0; i<config->protocol_count; i++)
	{
		add_int_to_iterator(&iterator, config->protocol[i], 1);
	}
	
	add_int_to_iterator(&iterator, config->payment_method_count, 1);
	for(i=0; i<config->payment_method_count; i++)
	{
		add_int_to_iterator(&iterator, config->payment_method[i], 1);
	}
	
	add_int_to_iterator(&iterator, config->energy_transfer_mode_count, 1);
	for(i=0; i<config->energy_transfer_mode_count; i++)
	{
		add_int_to_iterator(&iterator, config->energy_transfer_mode[i], 1);
	}
	
	add_int_to_iterator(&iterator, config->certificate_installation_support, 1);
	add_int_to_iterator(&iterator, config->certificate_update_support, 1);
	
	payload->len = iterator - payload->data;
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_configuration, payload);
	free_payload(&payload);
}

// Sets the configuration;
struct EVSE_config *v2gEvseGetConfiguration(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, v2g_mod_id, v2g_sub_evse_get_configuration, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return NULL;
	}
	
    struct EVSE_config *config = (struct EVSE_config *)malloc(sizeof(struct EVSE_config));
    config->code = payloadReaderReadInt(response, 1);
    if(config->code != 0)
    {
		return config;
	}
	
	config->evse_id_DIN_len = payloadReaderReadInt(response, 1);
	config->evse_id_DIN[config->evse_id_DIN_len] = 0;
	payloadReaderReadBytes(config->evse_id_DIN, response, config->evse_id_DIN_len);
	
	config->evse_id_ISO_len = payloadReaderReadInt(response, 1);
	config->evse_id_ISO[config->evse_id_ISO_len] = 0;
	payloadReaderReadBytes(config->evse_id_ISO, response, config->evse_id_ISO_len);
	
	config->protocol_count = payloadReaderReadInt(response, 1);
	payloadReaderReadBytes(config->protocol, response, config->protocol_count);
	
	config->payment_method[0] = 0;
	config->payment_method[1] = 0;
	config->payment_method_count = payloadReaderReadInt(response, 1);
	payloadReaderReadBytes(config->payment_method, response, config->payment_method_count);
	
	config->energy_transfer_mode_count = payloadReaderReadInt(response, 1);
	payloadReaderReadBytes(config->energy_transfer_mode, response, config->energy_transfer_mode_count);
	
	if(config->payment_method[0] + config->payment_method[1] != 0)
	{
		config->certificate_installation_support = payloadReaderReadInt(response, 1);
		config->certificate_update_support = payloadReaderReadInt(response, 1);
	}
	
    free_response(&response);
    return config;
}

// Set DC Charging Parameter
void v2gEvseSetDcChargingParameters(struct Framing *framing, struct EVSE_DC_param *param)
{
	struct Payload *payload = create_payload_empty(sizeof(struct EVSE_DC_param));
	uint8_t *iterator = payload->data;
	
	add_int_to_iterator(&iterator, param->isolation_level, 1);
	add_exponential_to_iterator(&iterator, param->min_voltage);
	add_exponential_to_iterator(&iterator, param->min_current);
	add_exponential_to_iterator(&iterator, param->max_voltage);
	add_exponential_to_iterator(&iterator, param->max_current);
	add_exponential_to_iterator(&iterator, param->max_power);
	
	if(param->current_regulation_tolerance == 0)
	{
		add_int_to_iterator(&iterator, 0, 1);
	}
	else
	{
		add_int_to_iterator(&iterator, 1, 1);
		add_exponential_to_iterator(&iterator, param->current_regulation_tolerance);
	}
	
	add_exponential_to_iterator(&iterator, param->peak_current_ripple);
	add_int_to_iterator(&iterator, param->status, 1);
	
	payload->len = iterator - payload->data;
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_dc_charging_parameters, payload);
	free_payload(&payload);
}

// Update DC Charging Parameter
void v2gEvseUpdateDcChargingParameters(struct Framing *framing, struct EVSE_DC_param *param)
{
	struct Payload *payload = create_payload_empty(sizeof(struct EVSE_DC_param));
	uint8_t *iterator = payload->data;
	add_int_to_iterator(&iterator, param->isolation_level, 1);
	add_exponential_to_iterator(&iterator, param->present_voltage);
	add_exponential_to_iterator(&iterator, param->present_current);
	if(param->max_voltage != 0)
	{
		add_int_to_iterator(&iterator, 1, 1);
		add_exponential_to_iterator(&iterator, param->max_voltage);
	}
	else
	{
		add_int_to_iterator(&iterator, 0, 1);
	}
	
	if(param->max_current != 0)
	{
		add_int_to_iterator(&iterator, 1, 1);
		add_exponential_to_iterator(&iterator, param->max_current);
	}
	else
	{
		add_int_to_iterator(&iterator, 0, 1);
	}
	
	if(param->max_power != 0)
	{
		add_int_to_iterator(&iterator, 1, 1);
		add_exponential_to_iterator(&iterator, param->max_power);
	}
	else
	{
		add_int_to_iterator(&iterator, 0, 1);
	}
	
	add_int_to_iterator(&iterator, param->status, 1);
	payload->len = iterator - payload->data;
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_update_dc_charging_parameters, payload);
	free_payload(&payload);
}

// 
struct EVSE_DC_param *v2gEvseGetDCChargingParameters(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, v2g_mod_id, v2g_sub_evse_get_dc_charging_parameters, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return NULL;
	}
	
	struct EVSE_DC_param *param = (struct EVSE_DC_param *)malloc(sizeof(struct EVSE_DC_param));
    param->code = payloadReaderReadInt(response, 1);
	if(param->code != 0)
	{
		return param;
	}
	
	param->isolation_level = payloadReaderReadInt(response, 1);
	param->min_voltage = payloadReaderReadExponential(response);
	param->min_current = payloadReaderReadExponential(response);
	param->max_voltage = payloadReaderReadExponential(response);
	param->max_current = payloadReaderReadExponential(response);
	param->max_power = payloadReaderReadExponential(response);
	
	param->current_regulation_tolerance = 0;
	if(payloadReaderReadInt(response, 1) == 1)
	{
		param->current_regulation_tolerance = payloadReaderReadExponential(response);
	}
	
	param->peak_current_ripple = payloadReaderReadExponential(response);
	
	param->present_voltage = 0;
	if(payloadReaderReadInt(response, 1) == 1)
	{
		param->present_voltage = payloadReaderReadExponential(response);
	}
	
	param->present_current = 0;
	if(payloadReaderReadInt(response, 1) == 1)
	{
		param->present_current = payloadReaderReadExponential(response);
	}
	
	param->status = payloadReaderReadInt(response, 1);
	
    free_response(&response);
    return param;
}

// Set AC Charging Parameter
void v2gEvseSetAcChargingParameters(struct Framing *framing, struct EVSE_AC_param *param)
{
	struct Payload *payload = create_payload_empty(sizeof(struct EVSE_AC_param));
	uint8_t *iterator = payload->data;
	
	add_int_to_iterator(&iterator, param->rcd_status, 1);
	add_exponential_to_iterator(&iterator, param->max_current);
	add_exponential_to_iterator(&iterator, param->nominal_voltage);
	
	payload->len = iterator - payload->data;
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_ac_charging_parameters, payload);
	free_payload(&payload);
}

// Set AC Charging Parameter
void v2gEvseUpdateAcChargingParameters(struct Framing *framing, struct EVSE_AC_param *param)
{
	struct Payload *payload = create_payload_empty(sizeof(struct EVSE_AC_param));
	uint8_t *iterator = payload->data;
	
	add_int_to_iterator(&iterator, param->rcd_status, 1);
	
	if(param->max_current != 0)
	{
		add_int_to_iterator(&iterator, 1, 1);
		add_exponential_to_iterator(&iterator, param->max_current);
	}
	else
	{
		add_int_to_iterator(&iterator, 0, 1);
	}
	
	payload->len = iterator - payload->data;
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_update_ac_charging_parameters, payload);
    free_payload(&payload);
}

struct EVSE_DC_param *v2gEvseGetAcChargingParameters(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, v2g_mod_id, v2g_sub_evse_get_ac_charging_parameters, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return NULL;
	}
	
	struct EVSE_DC_param *param = (struct EVSE_DC_param *)malloc(sizeof(struct EVSE_DC_param));
    param->code = payloadReaderReadInt(response, 1);
	if(param->code != 0)
	{
		return param;
	}
	
	param->rcd_status = payloadReaderReadInt(response, 1);
	param->nominal_voltage = payloadReaderReadExponential(response);
	param->max_current = payloadReaderReadExponential(response);
	
	free_response(&response);
    return param;
}

// Configures the SDP server;
void v2gEvseSetSdpConfig(struct Framing *framing, struct SPD_config *config)
{
	struct Payload *payload = create_payload_empty(sizeof(struct SPD_config));
	uint8_t *iterator = payload->data;
	
	add_int_to_iterator(&iterator, config->allow_unsecure, 1);
    if(config->allow_unsecure == 1)
    {
        add_int_to_iterator(&iterator, config->unsecure_port, 2);
    }
    
	add_int_to_iterator(&iterator, config->allow_secure, 1);
    if(config->allow_secure == 1)
    {
        add_int_to_iterator(&iterator, config->secure_port, 2);
    }
    
	payload->len = iterator - payload->data;
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_sdp_config, payload);
	free_payload(&payload);
}

// Returns the SDP server configuration;
struct SPD_config *v2gEvseGetSdpConfig(struct Framing *framing)
{
    struct Response *response = _sendReceiveAck(framing, v2g_mod_id, v2g_sub_evse_get_sdp_config, NULL);
	if(response == NULL)
	{
		framing->error = 1;
		return NULL;
	}
	
	struct SPD_config *config = (struct SPD_config *)malloc(sizeof(struct SPD_config));
    config->code = payloadReaderReadInt(response, 1);
    if(config->code != 0)
    {
		return config;
	}
	
    config->allow_unsecure = payloadReaderReadInt(response, 1);
    if(config->allow_unsecure == 1)
    {
        config->unsecure_port = payloadReaderReadInt(response, 2);
    }
	
	config->allow_secure = payloadReaderReadInt(response, 1);
    if(config->allow_secure == 1)
	{
        config->secure_port = payloadReaderReadInt(response, 2);
    }
    
	free_response(&response);
    return config;
}

void v2gEvseStartListen(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_start_listen, NULL);
}

void v2gEvseSetAuthorizationStatus(struct Framing *framing, uint8_t status)
{
	struct Payload *payload = create_payload8_t(status);
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_authorization_status, payload);
	free_payload(&payload);
}

void v2gEvseSetSchedules(struct Framing *framing, struct Schedule *schedules)
{
	struct Payload *payload = create_payload_empty(sizeof(struct Schedule));
	uint8_t *iterator = payload->data;

	int i, j, k, l;
	struct Schedule_profile profile;
	struct Schedule_type schedule_entrie;
	struct Sales_tariff_tuple tariff;
	struct Sales_tariff_entrie tariff_entrie;
	struct Consumption_cost consumption;
	struct Cost cost;
	
	add_int_to_iterator(&iterator, schedules->code, 1);
	add_int_to_iterator(&iterator, schedules->schedule_tuple_count, 1);
	
	for(i=0; i < schedules->schedule_tuple_count; i++)
	{
		profile = schedules->schedule_tuples[i];
		add_int_to_iterator(&iterator, profile.schedule_tuple_id, 2);
		add_int_to_iterator(&iterator, profile.schedule_entries_count, 2);
		
		for(j=0; j < profile.schedule_entries_count; j++)
		{
			schedule_entrie = profile.schedule_entries[j];
			add_int_to_iterator(&iterator, schedule_entrie.start, 4);
			add_int_to_iterator(&iterator, schedule_entrie.interval, 4);
			add_exponential_to_iterator(&iterator, schedule_entrie.power);
		}
	}
	
	if(schedules->energy_to_be_delivered != 0)
	{
		add_int_to_iterator(&iterator, 1, 1);
		add_exponential_to_iterator(&iterator, schedules->energy_to_be_delivered);
	}
	else
	{
		add_int_to_iterator(&iterator, 0, 1);
	}
	
	add_int_to_iterator(&iterator, schedules->sales_tariff_tuple_count, 1);
	
	for(i=0; i < schedules->sales_tariff_tuple_count; i++)
	{
		tariff = schedules->sales_tariff_tuples[i];
		add_int_to_iterator(&iterator, tariff.sales_tariff_id, 1);
		add_array_to_iterator(&iterator, tariff.sales_tariff_description, tariff.sales_tariff_description_len);
		add_int_to_iterator(&iterator, tariff.number_of_price_levels, 1);
		add_int_to_iterator(&iterator, tariff.sales_tariff_entrie_count, 2);
		
		for(j=0; j < tariff.sales_tariff_entrie_count; j++)
		{
			tariff_entrie = tariff.sales_tariff_entries[j];
			add_int_to_iterator(&iterator, tariff_entrie.time_interval_start, 4);
			add_int_to_iterator(&iterator, tariff_entrie.time_interval_duration, 4);
			add_int_to_iterator(&iterator, tariff_entrie.price_level, 1);
			add_int_to_iterator(&iterator, tariff_entrie.consumption_cost_count, 1);
			
			for(k=0; k < tariff_entrie.consumption_cost_count; k++)
			{
				consumption = tariff_entrie.consumption_costs[k];
				add_exponential_to_iterator(&iterator, consumption.start_value);
				add_int_to_iterator(&iterator, consumption.cost_count, 1);
				
				for(l=0; l < consumption.cost_count; l++)
				{
					cost = consumption.costs[l];
					add_int_to_iterator(&iterator, cost.kind, 1);
					add_int_to_iterator(&iterator, cost.amount, 4);
					add_int_to_iterator(&iterator, cost.amount_multiplier, 1);
				}
			}
		}
		
		add_array_to_iterator(&iterator, tariff.signature_id, tariff.signature_id_len);
		add_array_to_iterator(&iterator, tariff.digest_value, 32);
	}
	
	if(schedules->sales_tariff_tuple_count > 0)
	{
		add_array_to_iterator(&iterator, schedules->signature_value, 64);
	}
	
	payload->len = iterator - payload->data;
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_schedules, payload);
	free_payload(&payload);
}

void v2gEvseSetCableCheckFinished(struct Framing *framing, uint8_t code)
{
	struct Payload *payload = create_payload8_t(code);
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_cable_check_finished, payload);
	free_payload(&payload);
}

void v2gEvseStartCharging(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_start_charging, NULL);
}

void v2gEvseStopCharging(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_stop_charging, NULL);
}

void v2gEvseStopListen(struct Framing *framing)
{
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_stop_listen, NULL);
}

void v2gEvseSetCertificateInstallationAndUpdateResponse(struct Framing *framing, uint8_t status, uint8_t *exi, uint16_t exi_len)
{
	struct Payload *payload = create_payload_empty(exi_len + 1);
	uint8_t *iterator = payload->data;
	
	add_int_to_iterator(&iterator, status, 1);
	add_array_to_iterator(&iterator, exi, exi_len);
	
	_sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_cable_certificate_installation_and_update_response, payload);
	free_payload(&payload);
}

void v2gEvseSetMeterReceiptRequest(struct Framing *framing, struct Receipt *receipt)
{
	struct Payload *payload = create_payload_empty(sizeof(struct Receipt));
	uint8_t *iterator = payload->data;
	
	add_int_to_iterator(&iterator, receipt->meter_id_len, 1);
	add_array_to_iterator(&iterator, receipt->meter_id, receipt->meter_id_len);
	
	add_int_to_iterator(&iterator, receipt->meter_reading, 8);
	
	add_int_to_iterator(&iterator, receipt->meter_reading_signature_len, 1);
	add_array_to_iterator(&iterator, receipt->meter_reading_signature, receipt->meter_reading_signature_len);
	
	add_int_to_iterator(&iterator, receipt->meter_status, 2);
	add_int_to_iterator(&iterator, receipt->meter_timestamp, 8);
	
	payload->len = iterator - payload->data;
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_meter_receipt, payload);
	free_payload(&payload);
}

void v2gEvseSendNotification(struct Framing *framing, uint8_t renegotiation, uint16_t timeout)
{
    struct Payload *payload = create_payload_empty(sizeof(struct Notification));
	uint8_t *iterator = payload->data;
	add_int_to_iterator(&iterator, renegotiation, 1);
	add_int_to_iterator(&iterator, timeout, 2);
	
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_send_notification, payload);
	free_payload(&payload);
}

void v2gEvseSetSessionParameterTimeout(struct Framing *framing, uint16_t timeoutMs)
{
	struct Payload *payload = create_payload16_t(timeoutMs);
    _sendReceiveAck_responseless(framing, v2g_mod_id, v2g_sub_evse_set_session_parameter_timeout, payload);
	free_payload(&payload);
}

// Parse a session started message.
struct Session *v2gEvseParseSessionStarted(struct Framing *framing, struct Response *response)
{
	struct Session *session = (struct Session *)malloc(sizeof(struct Session));
	
    session->protocol = payloadReaderReadInt(response, 1);
    payloadReaderReadBytes(session->session_id, response, 8);
	
	session->evcc_id_len = payloadReaderReadInt(response, 1);
    payloadReaderReadBytes(session->evcc_id, response, session->evcc_id_len);
	
    payloadReaderFinalize(response);
    return session;
}

// Parse a payment selected message.
struct Payment *v2gEvseParsePaymentSelected(struct Framing *framing, struct Response *response)
{
    struct Payment *payment = (struct Payment *)malloc(sizeof(struct Payment));
    payment->selected_payment_method = payloadReaderReadInt(response, 1);
	
    if(payment->selected_payment_method == 1)
    {
		payment->contract_certificate_len = payloadReaderReadInt(response, 2);
        payloadReaderReadBytes(payment->contract_certificate, response, payment->contract_certificate_len);
		
		payment->mo_sub_ca1_len = payloadReaderReadInt(response, 2);
        payloadReaderReadBytes(payment->mo_sub_ca1, response, payment->mo_sub_ca1_len);
		
		payment->mo_sub_ca2_len = payloadReaderReadInt(response, 2);
        payloadReaderReadBytes(payment->mo_sub_ca2, response, payment->mo_sub_ca2_len);
		
		payment->emaid_len = payloadReaderReadInt(response, 1);
		payment->emaid[payment->emaid_len] = 0;
        payloadReaderReadBytes(payment->emaid, response, payment->emaid_len);
    }
	
    payloadReaderFinalize(response);
    return payment;
}

// Parse a authorization status requested message.
uint32_t v2gEvseParseAuthorizationStatusRequested(struct Framing *framing, struct Response *response)
{
	uint32_t timeout = payloadReaderReadInt(response, 4);
    payloadReaderFinalize(response);
    return timeout;
}

// Parse a session stopped message.
struct EVSE_id_request *v2gEvseParseRequestEvseId(struct Framing *framing, struct Response *response)
{
    struct EVSE_id_request *message = (struct EVSE_id_request *)malloc(sizeof(struct EVSE_id_request));
    message->timeout = payloadReaderReadInt(response, 4);
    message->format = payloadReaderReadInt(response, 1);
    payloadReaderFinalize(response);
    return message;
}

// Parse a energy transfer mode selected message.
struct EnergyTransfer *v2gEvseParseEnergyTransferModeSelected(struct Framing *framing, struct Response *response)
{
    struct EnergyTransfer *param = (struct EnergyTransfer *)malloc(sizeof(struct EnergyTransfer));
    
	param->departure_time = 0;
	param->energy_request = 0;
	param->min_current = 0;
	param->max_power = 0;
	param->energy_capacity = 0;
	param->full_soc = 0;
	param->bulk_soc = 0;
	
	//
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->departure_time = payloadReaderReadInt(response, 4);
    }
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->energy_request = payloadReaderReadExponential(response);
    }
    
    param->max_voltage = payloadReaderReadExponential(response);
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->min_current = payloadReaderReadExponential(response);
    }
    
    param->max_current = payloadReaderReadExponential(response);
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->max_power = payloadReaderReadExponential(response);
    }
    
    param->selected_energy_transfer_mode = payloadReaderReadInt(response, 1);
    if(param->selected_energy_transfer_mode <= 3)
    {
		param->energy_capacity = payloadReaderReadExponential(response);
    }
	
	if(payloadReaderReadInt(response, 1) == 1)
	{
		param->full_soc = payloadReaderReadInt(response, 1);
	}
	
	if(payloadReaderReadInt(response, 1) == 1)
	{
		param->bulk_soc = payloadReaderReadInt(response, 1);
	}
	
	param->ready = payloadReaderReadInt(response, 1);
	param->error_code = payloadReaderReadInt(response, 1);
	param->soc = payloadReaderReadInt(response, 1);
	
    payloadReaderFinalize(response);
    return param;
}

// Parse a request schedules message.
struct Schedule_request *v2gEvseParseSchedulesRequested(struct Framing *framing, struct Response *response)
{
    struct Schedule_request *message = (struct Schedule_request *)malloc(sizeof(struct Schedule_request));
    message->timeout = payloadReaderReadInt(response, 4);
    message->max_entries = payloadReaderReadInt(response, 2);
    payloadReaderFinalize(response);
    return message;
}

// Parse a dc charge parameteres changed message.
struct Charging_param *v2gEvseParseDCChargeParametersChanged(struct Framing *framing, struct Response *response)
{
    struct Charging_param *param = (struct Charging_param *)malloc(sizeof(struct Charging_param));
    param->max_voltage = payloadReaderReadExponential(response);
    param->max_current = payloadReaderReadExponential(response);
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->max_power = payloadReaderReadExponential(response);
    }
    
    param->ready = payloadReaderReadInt(response, 1);
    param->error_code = payloadReaderReadInt(response, 1);
    param->soc = payloadReaderReadInt(response, 1);
    
    param->target_voltage = payloadReaderReadExponential(response);
    param->target_current = payloadReaderReadExponential(response);
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->full_soc = payloadReaderReadInt(response, 1);
    }
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->bulk_soc = payloadReaderReadInt(response, 1);
    }
    
    param->charging_complete = payloadReaderReadInt(response, 1);
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->bulk_charging_complete = payloadReaderReadInt(response, 1);
    }
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->remaining_time_to_full_soc = payloadReaderReadExponential(response);
    }
    
    if(payloadReaderReadInt(response, 1) == 1)
    {
        param->remaining_time_to_bulk_soc = payloadReaderReadExponential(response);
    }
    
    payloadReaderFinalize(response);
    return param;
}

// Parse a dc charge parameteres changed message.
struct AC_charge_param *v2gEvseParseACChargeParametersChanged(struct Framing *framing, struct Response *response)
{
    struct AC_charge_param *param = (struct AC_charge_param *)malloc(sizeof(struct AC_charge_param));
	
    param->max_voltage = payloadReaderReadExponential(response);
    param->min_current = payloadReaderReadExponential(response);
    param->max_current = payloadReaderReadExponential(response);
    param->energy_amount = payloadReaderReadExponential(response);
    
    payloadReaderFinalize(response);
    return param;
}

// Parse a request cable check status message.
uint32_t v2gEvseParseCableCheckRequested(struct Framing *framing, struct Response *response)
{
    uint32_t timeout = payloadReaderReadInt(response, 4);
    payloadReaderFinalize(response);
    return timeout;
}

// Parse a start charging requested message.
struct Schedule_profile *v2gEvseParseStartChargingRequested(struct Framing *framing, struct Response *response)
{
    struct Schedule_profile *profile = (struct Schedule_profile *)malloc(sizeof(struct Schedule_profile ));
	
    profile->timeout = payloadReaderReadInt(response, 4);
    profile->schedule_tuple_id = payloadReaderReadInt(response, 2);
    profile->schedule_entries_count = payloadReaderReadInt(response, 1);
	
    for(int i=0; i < profile->schedule_entries_count; i++)
    {
        profile->schedule_entries->start = payloadReaderReadInt(response, 4);
        profile->schedule_entries->power = payloadReaderReadExponential(response);
    }
    
    payloadReaderFinalize(response);
    return profile;
}

// Parse a stop charging requested message.
struct Notification *v2gEvseParseStopChargingRequested(struct Framing *framing, struct Response *response)
{
    struct Notification *notification = (struct Notification *)malloc(sizeof(struct Notification));
    notification->timeout = payloadReaderReadInt(response, 4);
    notification->renegotiation = payloadReaderReadInt(response, 1);
	
    payloadReaderFinalize(response);
    return notification;
}

// Parse a session stopped message.
int8_t v2gEvseParseSessionStopped(struct Framing *framing, struct Response *response)
{
    uint8_t closure_type = payloadReaderReadInt(response, 1);
    payloadReaderFinalize(response);
    return closure_type;
}

// Parse a session error message.
uint8_t v2gEvseParseSessionError(struct Framing *framing, struct Response *response)
{
    uint8_t error_code = payloadReaderReadInt(response, 1);
    payloadReaderFinalize(response);
    return error_code;
}

// Parse a session error message.
struct EXI_request *v2gEvseParseCertificateRequested(struct Framing *framing, struct Response *response)
{
	struct EXI_request *exi = (struct EXI_request *)malloc(sizeof(struct EXI_request));
    exi->timeout = payloadReaderReadInt(response, 1);
    exi->exi_len = payloadReaderReadInt(response, 2);
    payloadReaderReadBytes(exi->data, response, exi->exi_len);
    
    payloadReaderFinalize(response);
    return exi;
}

// Parse a metering receipt status message.
uint8_t v2gEvseParseMeteringReceiptStatus(struct Framing *framing, struct Response *response)
{
    uint8_t status = payloadReaderReadInt(response, 1);   
    payloadReaderFinalize(response);
    return status;
}

// Receives a V2G request status message.
struct Response *v2gEvseReceiveRequest(struct Framing *framing)
{
    struct Response *response = _receive(framing, v2g_mod_id, 0x80, 0x91, 0xFF, 30000);
    return response;
}

// Receives a V2G request status message.
struct Response *v2gEvseReceiveRequestSilent(struct Framing *framing)
{
    struct Response *response = _receive(framing, v2g_mod_id, 0x80, 0x91, 0xFF, 100);
	return response;
}

// Receives a V2G request status message.
struct Response *v2gEvReceiveRequest(struct Framing *framing)
{
    struct Response *response = _receive(framing, v2g_mod_id, 0xC0, 0xCD, 0xFF, 1000);
    return response;
}