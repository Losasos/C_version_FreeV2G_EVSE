#ifndef EVSE_H
#define EVSE_H

#include "./whitebeet.h"
#include <windows.h>
#define DEFAULT_CONNECTIONG_TIMEOUT 60000

typedef struct Charger
{
	clock_t timestamp_last_calc_u;
	clock_t timestamp_last_calc_i;
	float evse_present_voltage;
	float evse_present_current;
	float evse_delta_u;
	float evse_delta_i;
	float evse_max_current;
	float evse_min_current;
	float evse_max_voltage;
	float evse_min_voltage;
	float evse_max_power;
	float ev_max_current;
	float ev_min_current;
	float ev_max_power;
	float ev_min_power;
	float ev_max_voltage;
	float ev_min_voltage;
	float ev_target_voltage;
	float ev_target_current;
	uint8_t is_charging;
	uint8_t is_stopped;
} Charger;

// handling messages recieved from EV as a part of charging loop
void _handleSessionStarted(struct Framing *framing, struct Response *response);
void _handlePaymentSelected(struct Framing *framing, struct Response *response);
void _handleRequestAuthorization(struct Framing *framing, struct Response *response);
uint8_t _handleEnergyTransferModeSelected(Framing *framing, Response *response, Charger *charger, EVSE_config *evse_config);
void _handleRequestSchedules(struct Framing *framing, struct Response *response, struct Schedule *schedule);
void _handleDCChargeParametersChanged(struct Framing *framing, struct Response *response, struct Charger *charger);
void _handleACChargeParametersChanged(struct Framing *framing, struct Response *response, struct Charger *charger);
void _handleRequestCableCheck(struct Framing *framing, struct Response *response);
void _handlePreChargeStarted(struct Framing *framing, struct Response *response, struct Charger *charger);
void _handleRequestStartCharging(struct Framing *framing, struct Response *response, struct Charger *charger);
void _handleRequestStopCharging(struct Framing *framing, struct Response *response, struct Charger *charger);
void _handleWeldingDetectionStarted(struct Framing *framing, struct Response *response);
void _handleSessionStopped(struct Framing *framing, struct Response *response, struct Charger *charger);
void _handleSessionError(struct Framing *framing, struct Response *response, struct Charger *charger);
void _handleCertificateInstallationRequested(struct Framing *framing, struct Response *response);
void _handleCertificateUpdateRequested(struct Framing *framing, struct Response *response);
void _handleMeteringReceiptStatus(struct Framing *framing, struct Response *response);

// charging loop
void _handleNetworkEstablished(struct Framing *framing, EVSE_config *evse_config, Charger *charger, Schedule *schedule);
// loop, waiting for another device to connect
uint8_t _waitEvConnected(struct Framing *framing, size_t timeout);
// SLAC
uint8_t _handleEvConnected(struct Framing *framing);
// connection 
int8_t _initialize(struct Framing *framing);
// main evse function
uint8_t start_evse(struct Framing *framing, EVSE_config *evse_config, Charger *charger, Schedule *schedule);
#endif