#ifndef WHITEBEAT_H
#define WHITEBEAT_H

#include "./wb_framing.h"
#define DC_UPDATEBLE_PARAM_LENGTH 19
#define NMK_SIZE 16
#define NID_SIZE 7

// Datastructutre definition
//////////////////////////////
typedef struct EV_config
{
	float battery_capacity;
	uint8_t evid[6];
	uint8_t protocol_count;
	uint8_t protocol[2];
	uint8_t payment_method_count;
	uint8_t payment_method[2];
	uint8_t energy_transfer_mode_count;
	uint8_t energy_transfer_mode[2];
} EV_config;

typedef struct EV_DC_param
{
	float min_voltage;
	float min_current;
	float min_power;
	float max_voltage;
	float max_current;
	float max_power;
	float target_voltage;
	float target_current;
	float energy_request;
	uint32_t departure_time;
	uint8_t status;
	uint8_t soc;
	uint8_t full_soc;
	uint8_t bulk_soc;
} EV_DC_param;

typedef struct EV_AC_param
{
	float min_voltage;
	float min_current;
	float min_power;
	float max_voltage;
	float max_current;
	float max_power;
	float target_voltage;
	float target_current;
	float energy_request;
	uint32_t departure_time;
} EV_AC_param;

typedef struct Profile
{
	int16_t schedule_tuple_id;
	uint8_t charging_profile_entries_count;
	uint32_t start[24];
	uint32_t interval[24];
	float power[24];
} Profile;

typedef struct EV_session_started
{
	uint8_t session_id[8];
	uint8_t evid[6];
	uint8_t protocol;
	uint8_t evse_id_len;
	char evse_id[38];
	uint8_t payment_method;
	uint8_t energy_transfer_mode;
} EV_session_started;

typedef struct EVSE_DC_param
{
	float min_voltage;
	float max_voltage;
	float nominal_voltage;
	float min_current;
	float max_current;
	float max_power;
	float present_voltage;
	float present_current;
	float peak_current_ripple;
	float current_regulation_tolerance;
	//uint8_t is_tolerance;
	uint8_t evid[6];
	uint8_t status;
	uint8_t rcd_status;
	uint8_t code;
	uint8_t isolation_level;
} EVSE_DC_param;

typedef struct EVSE_AC_param
{
	float max_current;
	float nominal_voltage;
	uint8_t rcd_status;
	uint8_t code;
} EVSE_AC_param;

typedef struct SPD_config
{
	uint16_t unsecure_port;
	uint16_t secure_port;
	uint8_t allow_unsecure;
	uint8_t allow_secure;
	uint8_t code;
} SPD_config;

typedef struct DC_charge_param
{
	float evse_min_voltage;
	float evse_min_current;
	float evse_min_power;
	float evse_max_voltage;
	float evse_max_current;
	float evse_max_power;
	float evse_present_voltage;
	float evse_present_current;
	float evse_peak_current_ripple;
	float evse_current_regulation_tolerance;
	float evse_energy_to_be_delivered;
	uint8_t evse_status;
	uint8_t evse_isolation_status;
	uint8_t evse_voltage_limit_achieved;
	uint8_t evse_current_limit_achieved;
	uint8_t evse_power_limit_achieved;
} DC_charge_param;

typedef struct AC_charge_param
{
	float nominal_voltage;
	float max_voltage;
	float min_current;
	float max_current;
	float energy_amount;
	uint8_t rcd;
} AC_charge_param;

typedef struct EV_notification
{
	uint16_t max_delay;
	uint8_t type;
} EV_notification;

typedef struct EVSE_config
{
	uint8_t code;
	uint8_t evse_id_DIN_len;
	char evse_id_DIN[39];
	uint8_t evse_id_ISO_len;
	char evse_id_ISO[39];
	uint8_t protocol_count; 
	uint8_t protocol[2];
	uint8_t payment_method_count;
	uint8_t payment_method[2];
	uint8_t energy_transfer_mode_count;
	uint8_t energy_transfer_mode[6];
	uint8_t certificate_installation_support;
	uint8_t certificate_update_support;
} EVSE_config;

typedef struct Session
{
	uint8_t protocol;
	uint8_t session_id[8];
	uint8_t evcc_id_len;
	uint8_t evcc_id[20];
} Session;

typedef struct Payment
{
	uint16_t contract_certificate_len;
	uint16_t mo_sub_ca1_len;
	uint16_t mo_sub_ca2_len;
	uint8_t selected_payment_method;
    uint8_t contract_certificate[800]; //798
    uint8_t mo_sub_ca1[800]; //798
    uint8_t mo_sub_ca2[800]; //798
	uint8_t emaid_len;
    char emaid[15];
} Payment;

typedef struct Notification
{
	uint16_t timeout;
	uint8_t renegotiation;
} Notification;

typedef struct Schedule_request
{
	uint32_t timeout;
    uint16_t max_entries;
} Schedule_request;

typedef struct Receipt
{
	uint8_t meter_id_len;
	char meter_id[32];
	uint64_t meter_reading;
	uint8_t meter_reading_signature_len;
	uint8_t meter_reading_signature[64];
	int16_t meter_status;
	int64_t meter_timestamp;
} Receipt;

typedef struct EVSE_id_request
{
	uint32_t timeout;
    uint8_t format;
} EVSE_id_request;

typedef struct EXI_request
{
	uint32_t timeout;
    uint16_t exi_len;
	uint8_t data[6000]; //5998
} EXI_request;

typedef struct Charging_param
{
    float max_voltage;
    float max_current;
    float max_power;
	float target_voltage;
    float target_current;
	float remaining_time_to_full_soc;
    float remaining_time_to_bulk_soc;
    uint8_t ready;
    uint8_t error_code;
    uint8_t soc;
    uint8_t full_soc;
    uint8_t bulk_soc;
    uint8_t charging_complete;
    uint8_t bulk_charging_complete;
} Charging_param;

typedef struct EnergyTransfer
{
	uint32_t departure_time;
	float energy_request;
	float min_current;
	float max_voltage;
    float max_current;
    float max_power;
    uint8_t ready;
    uint8_t error_code;
    uint8_t soc;
    uint8_t full_soc;
    uint8_t bulk_soc;
	uint8_t selected_energy_transfer_mode;
	uint8_t energy_capacity;
} EnergyTransfer;

// sub-structs for Schedule struct
///////////////////////////////////////
typedef struct Schedule_type
{
	uint32_t start;
	uint32_t interval;
	float power;
} Schedule_type;

typedef struct Schedule_profile
{
	uint32_t timeout;
	int16_t schedule_tuple_id;
	uint16_t schedule_entries_count;
	struct Schedule_type schedule_entries[20];
	uint8_t tuple_number;
} Schedule_profile;

typedef struct Cost
{
	uint8_t kind;
	uint32_t amount;
	int8_t amount_multiplier;
} Cost;

typedef struct Consumption_cost
{
	float start_value;
	uint8_t cost_count;
	struct Cost costs[3];
} Consumption_cost;

typedef struct Sales_tariff_entrie
{
	uint32_t time_interval_start;
	uint32_t time_interval_duration;
	uint8_t price_level;
	uint8_t consumption_cost_count;
	struct Consumption_cost consumption_costs[3];
} Sales_tariff_entrie;

typedef struct Sales_tariff_tuple
{
	uint8_t sales_tariff_id;
	uint8_t sales_tariff_description_len;
	char sales_tariff_description[33];
	uint8_t number_of_price_levels;
	uint16_t sales_tariff_entrie_count;
	struct Sales_tariff_entrie sales_tariff_entries[20];
	uint8_t signature_id_len;
	char signature_id[255];
	uint8_t digest_value[32];
} Sales_tariff_tuple;

typedef struct Schedule
{
	uint8_t code;
	uint8_t schedule_tuple_count;
	struct Schedule_profile schedule_tuples[3];
	float energy_to_be_delivered;
	uint8_t sales_tariff_tuple_count;
	struct Sales_tariff_tuple sales_tariff_tuples[3];
	uint8_t signature_value_len;
	uint8_t signature_value[64];
} Schedule;

//////////////////////////////
// Utility functions definition
//////////////////////////////
float payloadReaderReadExponential(struct Response *response);
size_t payloadReaderReadInt(struct Response *response, size_t num);
int8_t payloadReaderReadBytes(void *dest, struct Response *response, size_t num);
int8_t payloadReaderFinalize(struct Response *response);
void payloadReaderInitialize(struct Response *response);

// WHITE-BEET functions definition
//////////////////////////////
int systemGetVersion(struct Framing *framing, char **version);

int8_t controlPilotSetMode(struct Framing *framing, uint8_t mode);// TESTED
int16_t controlPilotGetMode(struct Framing *framing);
int8_t controlPilotStart(struct Framing* framing);// TESTED
uint8_t controlPilotStop(struct Framing* framing);

int8_t controlPilotSetDutyCycle(struct Framing *framing, float duty_cycle_in);// TESTED
float controlPilotGetDutyCycle(struct Framing *framing);
int8_t controlPilotSetResistorValue(struct Framing *framing, uint8_t value);
int8_t controlPilotGetResistorValue(struct Framing *framing);
int8_t controlPilotGetState(struct Framing *framing);// TESTED

int8_t slacStart(struct Framing *framing, uint8_t mode_in);// TESTED
int8_t slacStop(struct Framing *framing);
void slacStartMatching(struct Framing *framing);// TESTED
int8_t slacMatched(struct Framing *framing);// TESTED
int8_t networkConfigSetPortMirrorState(struct Framing *framing, uint8_t value);
int8_t slacJoinNetwork(struct Framing *framing, uint8_t *nid, uint8_t *nmk);
int8_t slacJoined(struct Framing *framing);
int8_t slacSetValidationConfiguration(struct Framing *framing, uint8_t configuration);

int8_t v2gSetMode(struct Framing *framing, uint8_t mode);// TESTED
int8_t v2gGetMode(struct Framing *framing);
void v2gStart(struct Framing *framing);
void v2gStop(struct Framing *framing);

// in developement
/////////////////////

void v2gEvSetConfiguration(struct Framing *framing, struct EV_config *config);
struct EV_config *v2gEvGetConfiguration(struct Framing *framing);

void v2gSetDCChargingParameters(struct Framing *framing, struct EV_DC_param *params);
void v2gUpdateDCChargingParameters(struct Framing *framing, struct EV_DC_param *params);
struct EV_DC_param *v2gGetDCChargingParameters(struct Framing *framing);

void v2gSetACChargingParameters(struct Framing *framing, struct EV_AC_param *parameter);
void v2gUpdateACChargingParameters(struct Framing *framing, struct EV_AC_param *parameter);
struct EV_AC_param *v2gACGetChargingParameters(struct Framing *framing);

void v2gSetChargingProfile(struct Framing *framing, struct Profile *schedule);
void v2gStartSession(struct Framing *framing);
void v2gStartCableCheck(struct Framing *framing);
void v2gStartPreCharging(struct Framing *framing);
void v2gStartCharging(struct Framing *framing);
void v2gStopCharging(struct Framing *framing, uint8_t renegotiation);
void v2gStopSession(struct Framing *framing);

struct EV_session_started *v2gEvParseSessionStarted(struct Framing *framing, struct Response *response);
struct DC_charge_param *v2gEvParseDCChargeParametersChanged(struct Framing *framing, struct Response *response);
struct AC_charge_param *v2gEvParseACChargeParametersChanged(struct Framing *framing, struct Response *response);
struct Schedule_profile *v2gEvParseScheduleReceived(struct Framing *framing, struct Response *response);
struct EV_notification *v2gEvParseNotificationReceived(struct Framing *framing, struct Response *response);
int8_t v2gEvParseSessionError(struct Framing *framing, struct Response *response);

void v2gEvseSetConfiguration(struct Framing *framing, struct EVSE_config *config);// TESTED
struct EVSE_config *v2gEvseGetConfiguration(struct Framing *framing);
void v2gEvseSetDcChargingParameters(struct Framing *framing, struct EVSE_DC_param *param);// TESTED
void v2gEvseUpdateDcChargingParameters(struct Framing *framing, struct EVSE_DC_param *param);// TESTED
struct EVSE_DC_param *v2gEvseGetDCChargingParameters(struct Framing *framing);
void v2gEvseSetAcChargingParameters(struct Framing *framing, struct EVSE_AC_param *param);// TESTED
void v2gEvseUpdateAcChargingParameters(struct Framing *framing, struct EVSE_AC_param *param);
struct EVSE_DC_param *v2gEvseGetAcChargingParameters(struct Framing *framing);

void v2gEvseSetSdpConfig(struct Framing *framing, struct SPD_config *config);
struct SPD_config *v2gEvseGetSdpConfig(struct Framing *framing);
void v2gEvseStartListen(struct Framing *framing);// TESTED
void v2gEvseSetAuthorizationStatus(struct Framing *framing, uint8_t status);// TESTED
void v2gEvseSetSchedules(struct Framing *framing, struct Schedule *schedules);// TESTED
void v2gEvseSetCableCheckFinished(struct Framing *framing, uint8_t code);// TESTED
void v2gEvseStartCharging(struct Framing *framing);// TESTED
void v2gEvseStopCharging(struct Framing *framing);// TESTED
void v2gEvseStopListen(struct Framing *framing);
void v2gEvseSetCertificateInstallationAndUpdateResponse(struct Framing *framing, uint8_t status, uint8_t *exi, uint16_t exi_len);
void v2gEvseSetMeterReceiptRequest(struct Framing *framing, struct Receipt *receipt);
void v2gEvseSendNotification(struct Framing *framing, uint8_t renegotiation, uint16_t timeout);

void v2gEvseSetSessionParameterTimeout(struct Framing *framing, uint16_t timeoutMs);
struct Session *v2gEvseParseSessionStarted(struct Framing *framing, struct Response *response);// TESTED
struct Payment *v2gEvseParsePaymentSelected(struct Framing *framing, struct Response *response);// TESTED
uint32_t v2gEvseParseAuthorizationStatusRequested(struct Framing *framing, struct Response *response);// TESTED
struct EVSE_id_request *v2gEvseParseRequestEvseId(struct Framing *framing, struct Response *response);
struct EnergyTransfer *v2gEvseParseEnergyTransferModeSelected(struct Framing *framing, struct Response *response);// TESTED
struct Schedule_request *v2gEvseParseSchedulesRequested(struct Framing *framing, struct Response *response);// TESTED
struct Charging_param *v2gEvseParseDCChargeParametersChanged(struct Framing *framing, struct Response *response);// TESTED
struct AC_charge_param *v2gEvseParseACChargeParametersChanged(struct Framing *framing, struct Response *response);
uint32_t v2gEvseParseCableCheckRequested(struct Framing *framing, struct Response *response);// TESTED
struct Schedule_profile *v2gEvseParseStartChargingRequested(struct Framing *framing, struct Response *response);// TESTED
struct Notification *v2gEvseParseStopChargingRequested(struct Framing *framing, struct Response *response);// TESTED
int8_t v2gEvseParseSessionStopped(struct Framing *framing, struct Response *response);// TESTED
uint8_t v2gEvseParseSessionError(struct Framing *framing, struct Response *response);// TESTED
struct EXI_request *v2gEvseParseCertificateRequested(struct Framing *framing, struct Response *response);
uint8_t v2gEvseParseMeteringReceiptStatus(struct Framing *framing, struct Response *response);

struct Response *v2gEvseReceiveRequest(struct Framing *framing);// TESTED
struct Response *v2gEvseReceiveRequestSilent(struct Framing *framing);// TESTED
struct Response *v2gEvReceiveRequest(struct Framing *framing);
#endif