////////////////////////////////
// This file is EVSE side initialisation and logic loop

#include "./evse.h"

const char* error_messages[] = {
	"Unspecified",
	"Sequence error",
	"Service ID invalid",
	"Unknown session",
	"Service selection invalid",
	"Payment selection invalid",
	"Certificate expired",
	"Signature Error",
	"No certificate available",
	"Certificate chain error",
	"Challenge invalid",
	"Contract canceled",
	"Wrong charge parameter",
	"Power delivery not applied",
	"Tariff selection invalid",
	"Charging profile invalid",
	"Present voltage too low",
	"Metering signature not valid",
	"No charge service selected",
	"Wrong energy transfer type",
	"Contactor error",
	"Certificate not allowed at this EVSE",
	"Certificate revoked",
	"Charge parameter timeout reached"
};

// This will handle a complete charging session of the EVSE.
uint8_t start_evse(struct Framing *framing, EVSE_config *evse_config, Charger *charger, Schedule *schedule)
{
	if(!_initialize(framing))
	{
		return 0;
	}
	
	if(!_waitEvConnected(framing, DEFAULT_CONNECTIONG_TIMEOUT))
    {
        return 0;
	}
	
	if(!_handleEvConnected(framing))
	{
        return 0;
	}
	_handleNetworkEstablished(framing, evse_config, charger, schedule);
}

// Initializes the whitebeet by setting the control pilot mode and setting the duty cycle
// to 100%. The SLAC module is also started. This one needs ~1s to 2s to be ready.
// Therefore we delay the initialization by 2s.
int8_t _initialize(struct Framing *framing)
{
	uint8_t status = 0;
    printf("Set the CP mode to EVSE\n");
    status |= controlPilotSetMode(framing, 1) << 0;
    printf("Set the CP duty cycle to 100%\n");
    status |= controlPilotSetDutyCycle(framing, 100) << 1;
    printf("Start the CP service\n");
    status |= controlPilotStart(framing) << 2;
    printf("Start SLAC in EVSE mode\n");
    status |= slacStart(framing, 1) << 3;
	
	if(status != 15)
	{
		printf("Initialisation FAILD! \n");
		printf("Code: %d \n", status);
		return 0;
	}
	return 1;
}

// We check for the state on the CP. When there is no EV connected we have state A on CP.
// When an EV connects the state changes to state B and we can continue with further steps.
uint8_t _waitEvConnected(struct Framing *framing, size_t timeout)
{
	// Timeout is only for demo aplication
	// The real EVSE should wait until an EV connects indefinitely
	// sending sending beacons via controlPilotGetState() all the time
    clock_t timestamp_start = clock() + timeout;
	printf("Wait until an EV connects...\n");
	printf("Timeout in: %d sec\n", (int)(timeout/1000));
	
	int8_t cp_state;
	while(1)
	{
		cp_state = controlPilotGetState(framing);
		
		if(cp_state == 1)
		{
			printf("EV connected\n");
			return 1;
		}
		
		if(timestamp_start < clock())
		{
			printf("Timeout\n");
			return 0;
		}
		
		if(cp_state != 0 && cp_state != -1)
		{
			printf("CP in wrong state: %d\n", cp_state);
			return 0;
		}
	}
}

// When an EV connected we start our start matching process of the SLAC which will be ready
// to answer SLAC request for the next 50s. After that we set the duty cycle on the CP to 5%
// which indicates that the EV can start with sending SLAC requests.
uint8_t _handleEvConnected(struct Framing *framing)
{
    printf("Start SLAC matching\n");
    slacStartMatching(framing);
    printf("Set duty cycle to 5%\n");
    controlPilotSetDutyCycle(framing, 5);
	printf("Geting match status...\n");
	if(slacMatched(framing) != 1)
	{
		printf("SLAC matching failed\n");
		return 0;
	}
	printf("SLAC matching successful\n");
	return 1;
}

// When SLAC was successful we can start the V2G module. Set our supported protocols,
// available payment options and energy transfer modes. Then we start waiting for
// notifications for requested parameters.
void _handleNetworkEstablished(struct Framing *framing, EVSE_config *evse_config, Charger *charger, Schedule *schedule)
{
    v2gSetMode(framing, 1);
	
	struct EVSE_DC_param dc_charging_parameters;
	dc_charging_parameters.isolation_level = 1;
	dc_charging_parameters.min_voltage = charger->evse_min_voltage;
	dc_charging_parameters.min_current = charger->evse_min_current;
	dc_charging_parameters.max_voltage = charger->evse_max_voltage;
	dc_charging_parameters.max_current = charger->evse_max_current;
	dc_charging_parameters.max_power = charger->evse_max_power;
	dc_charging_parameters.current_regulation_tolerance = 0;
	dc_charging_parameters.peak_current_ripple = charger->evse_delta_i;
	dc_charging_parameters.status = 0;
	
    struct EVSE_AC_param ac_charging_parameters;
	ac_charging_parameters.rcd_status = 0;
	ac_charging_parameters.nominal_voltage = charger->evse_max_voltage;
	ac_charging_parameters.max_current = charger->evse_max_current;
	
	v2gEvseSetConfiguration(framing, evse_config);
	v2gEvseSetDcChargingParameters(framing, &dc_charging_parameters);
    v2gEvseSetAcChargingParameters(framing, &ac_charging_parameters);
	
    printf("Start V2G\n");
    v2gEvseStartListen(framing);

	struct EVSE_DC_param charging_parameters;
	struct Response *response = NULL;
	uint8_t loop = 1;

    while(loop)
    {
		free(response);
        if(charger->is_charging == 1)
        {
            response = v2gEvseReceiveRequestSilent(framing);
			charging_parameters.isolation_level = 1;
			charging_parameters.present_voltage = charger->evse_present_voltage;
			charging_parameters.present_current = charger->evse_present_current;
			charging_parameters.max_voltage = charger->evse_max_voltage;
			charging_parameters.max_current = charger->evse_max_current;
			charging_parameters.max_power = charger->evse_max_power;
			charging_parameters.status = 0;
			v2gEvseUpdateDcChargingParameters(framing, &charging_parameters);
        }
        else
        {
            response = v2gEvseReceiveRequest(framing);
        }
		
        if(response == NULL || response->sub_id == 0)
        {
            continue;
        }
		#ifdef OUTPUT
		printf("====================\n");
		printf("debug, handling: %0X\n", response->sub_id);
		printf("====================\n");
		#endif
		
		switch(response->sub_id)
		{
		case 0x80:
			_handleSessionStarted(framing, response);// TESTED
			break;
		case 0x81:
			_handlePaymentSelected(framing, response);// TESTED
			break;
		case 0x82:
			_handleRequestAuthorization(framing, response);// TESTED
			break;
		case 0x83:
			_handleEnergyTransferModeSelected(framing, response, charger, evse_config);// TESTED
			break;
		case 0x84:
			_handleRequestSchedules(framing, response, schedule);// TESTED
			break;
		case 0x85:
			_handleDCChargeParametersChanged(framing, response, charger);// TESTED
			break;
		case 0x86:
			_handleACChargeParametersChanged(framing, response, charger);
			break;
		case 0x87:
			_handleRequestCableCheck(framing, response);// TESTED
			break;
		case 0x88:
			_handlePreChargeStarted(framing, response, charger);// TESTED
			break;
		case 0x89:
			_handleRequestStartCharging(framing, response, charger);// TESTED
			break;
		case 0x8A:
			_handleRequestStopCharging(framing, response, charger);// TESTED
			break;
		case 0x8B:
			_handleWeldingDetectionStarted(framing, response);
			break;
		case 0x8C:
			_handleSessionStopped(framing, response, charger);// TESTED
			loop = 0;
			break;
		case 0x8D:
			// not defined
			break;
		case 0x8E:
			_handleSessionError(framing, response, charger);// TESTED
			break;
		case 0x8F:
			_handleCertificateInstallationRequested(framing, response);
			break;
		case 0x90:
			_handleCertificateUpdateRequested(framing, response);
			break;
		case 0x91:
			_handleMeteringReceiptStatus(framing, response);
			break;
		default:
			#ifdef OUTPUT
			printf("Message ID not supported: %02x\n", response->sub_id);
			#endif
			loop = 0;
		}
    }
	
	free(response);
    v2gEvseStopListen(framing);
}

///////////////////////////////////////////////////////////////////////////////
// Handler functions

// Handle the SessionStarted notification
void _handleSessionStarted(struct Framing *framing, struct Response *response)
{
    struct Session *session = v2gEvseParseSessionStarted(framing, response);
	printf("\"Session started\" received\n");
    printf("Protocol: %d\n", session->protocol);
    printf("Session ID: %x\n", session->session_id);
    printf("EVCC ID: %x\n", session->evcc_id);
	free(session);
}

// Handle the PaymentSelected notification
void _handlePaymentSelected(struct Framing *framing, struct Response *response)
{
    struct Payment *payment = v2gEvseParsePaymentSelected(framing, response);
	printf("\"Payment selcted\" received\n");
    printf("Selected payment method: %d\n", payment->selected_payment_method);
    if(payment->selected_payment_method == 1)
    {
        printf("Contract certificate: %x\n", payment->contract_certificate);
        printf("mo_sub_ca1: %x\n", payment->mo_sub_ca1);
        printf("mo_sub_ca2: %x\n", payment->mo_sub_ca2);
        printf("EMAID: %x\n", payment->emaid);
    }
	free(payment);
}

// Handle the RequestAuthorization notification.
// The authorization status will be requested from the user.
void _handleRequestAuthorization(struct Framing *framing, struct Response *response)
{
    printf("\"Request Authorization\" received\n");
    uint32_t timeout_millisec = v2gEvseParseAuthorizationStatusRequested(framing, response);
	clock_t time_start;
    // Promt for authorization status
	printf("Authorize the vehicle? y/n: ");
	char input = 0;
	
	time_start = clock() + timeout_millisec;
	do
	{
		if(GetAsyncKeyState('N'))
		{
			input = 1;
			break;
		}
		if(GetAsyncKeyState('Y'))
		{
			input = 0;
			break;
		}
	} while(time_start > clock());
    
    if(input == 1)
    {
		printf("\nVehicle was NOT authorized by user!\n");
		v2gEvseSetAuthorizationStatus(framing, 1);
    }
    else
    {
        printf("\nVehicle was authorized by user!\n");
		v2gEvseSetAuthorizationStatus(framing, 0);
    }
}

// Handle the energy transfer mode selected notification
uint8_t _handleEnergyTransferModeSelected(Framing *framing, Response *response, Charger *charger, EVSE_config *evse_config)
{
    charger->is_charging = 1;
    struct EnergyTransfer *param = v2gEvseParseEnergyTransferModeSelected(framing, response);
    
	#ifdef OUTPUT
	printf("\"Energy transfer mode selected\" received\n");
	printf("Departure time: %f\n", param->departure_time);
	printf("Energy request: %f\n", param->energy_request);
    printf("Maximum voltage: %f\n", param->max_voltage);
	printf("Minimum current: %f\n", param->min_current);
    printf("Maximum current: %f\n", param->max_current);
	printf("Maximum power: %f\n", param->max_power);
	printf("Energy Capacity: %f\n", param->energy_capacity);	
	printf("Full SoC: %f\n", param->full_soc);
	printf("Bulk SoC: %f\n", param->bulk_soc);
    printf("Ready: %d\n", param->ready);
    printf("Error code: %f\n", param->error_code);
    printf("SoC: %f\n", param->soc);
	printf("Selected energy transfer mode: %f\n", param->selected_energy_transfer_mode);
	#endif
	
	charger->ev_max_voltage = param->max_voltage;
	charger->ev_min_current = param->min_current;
	charger->ev_max_current = param->max_current;
	charger->ev_max_power = param->max_power;
	
	charger->evse_max_voltage = param->max_voltage;
	charger->evse_min_current = param->min_current;
    charger->evse_max_current = param->max_current;
	charger->evse_max_power = param->max_power;
	
	for(int i=0; i<evse_config->energy_transfer_mode_count; i++)
	{
		if(param->selected_energy_transfer_mode == evse_config->energy_transfer_mode[i])
		{
			free(param);
			return 1;
		}
	}

	#ifdef OUTPUT
	printf("Energy transfer mode mismatch!\n");
	#endif
	v2gEvseStopCharging(framing);
	free(param);
	return 0;
}

// Handle the RequestSchedules notification
void _handleRequestSchedules(struct Framing *framing, struct Response *response, struct Schedule *schedule)
{
    printf("\"Request Schedules\" received\n");
    struct Schedule_request *message = v2gEvseParseSchedulesRequested(framing, response);
    printf("Max entries: %d\n", message->max_entries);
	
    printf("Set the schedule: %d\n", schedule);
	v2gEvseSetSchedules(framing, schedule);
	free(message);
}

// Handle the DCChargeParametersChanged notification
void _handleDCChargeParametersChanged(struct Framing *framing, struct Response *response, struct Charger *charger)
{
    struct Charging_param *param = v2gEvseParseDCChargeParametersChanged(framing, response);
	
	#ifdef OUTPUT
    printf("\"DC Charge Parameters Changed\" received\n");
    printf("EV maximum current: %f A\n", param->max_current);
    printf("EV maximum voltage: %f V\n", param->max_voltage);
	printf("EV maximum power: %f W\n", param->max_power);
    printf("EV ready: %d \n", param->ready);
    printf("Error code: %d \n", param->error_code);
    printf("SOC: %d %\n", param->soc);
	printf("EV target voltage: %f V\n", param->target_voltage);
	printf("EV target current: %f A\n", param->target_current);
	printf("Charging complete: %d \n", param->charging_complete);
	printf("Bulk charging complete: %d \n", param->bulk_charging_complete);
	printf("Remaining time to full SOC: %f s\n", param->remaining_time_to_full_soc);
	printf("Remaining time to bulk SOC: %f s\n", param->remaining_time_to_bulk_soc);
	#endif
	
	charger->ev_max_current = param->max_current;
	charger->ev_max_voltage = param->max_voltage;
	charger->ev_max_power = param->max_power;
	charger->ev_target_voltage = param->target_voltage;
	charger->ev_target_current = param->target_current;
	
	charger->evse_max_voltage = param->max_voltage;
    charger->evse_max_current = param->max_current;
	charger->evse_max_power = param->max_power;
	charger->evse_present_voltage = charger->ev_target_voltage;
	charger->evse_present_current = charger->ev_target_current;
	
	free(param);
	
	/*
		update charging target_values on a charging controller
		get measurement values and set parameters to be send back to an EV
		(real measurements)
	*/
	
    struct EVSE_DC_param charging_parameters;
	charging_parameters.isolation_level = 1;
	charging_parameters.present_voltage = charger->evse_present_voltage;
	charging_parameters.present_current = charger->evse_present_current;
	charging_parameters.max_voltage = charger->evse_max_voltage;
	charging_parameters.max_current = charger->evse_max_current;
	charging_parameters.max_power = charger->evse_max_power;
	charging_parameters.status = 0;
	
	v2gEvseUpdateDcChargingParameters(framing, &charging_parameters);
}

// Handle the ACChargeParametersChanged notification
void _handleACChargeParametersChanged(struct Framing *framing, struct Response *response, struct Charger *charger)
{
    struct AC_charge_param *param = v2gEvseParseACChargeParametersChanged(framing, response);
	
    #ifdef OUTPUT
    printf("\"AC Charge Parameters Changed\" received\n");
    printf("EV maximum voltage: %dV\n", param->max_voltage);
    printf("EV minimum current: %dW\n", param->min_current);
    printf("EV maximum current: %dA\n", param->max_current);
    printf("Energy amount: %dA\n", param->energy_amount);
	#endif
	
	charger->ev_max_voltage = param->max_voltage;
	charger->ev_min_current = param->min_current;
    charger->ev_max_current = param->max_current;
	
	charger->evse_max_voltage = param->max_voltage;
	charger->evse_min_current = param->min_current;
    charger->evse_max_current = param->max_current;
	free(param);
	
	/*
		update charging target_values on a charging controller
		get measurement values and set parameters to be send back to an EV
		(real measurements)
	*/
    
    struct EVSE_AC_param charging_parameters;
	charging_parameters.rcd_status = 0;
    charging_parameters.max_current = charger->evse_max_current;
	
	v2gEvseUpdateAcChargingParameters(framing, &charging_parameters);
}

// Handle the RequestCableCheck notification
void _handleRequestCableCheck(struct Framing *framing, struct Response *response)
{
    printf("\"Request Cable Check Status\" received\n");
    uint32_t timeout = v2gEvseParseCableCheckRequested(framing, response);
	v2gEvseSetCableCheckFinished(framing, 0);
}

// Handle the PreChargeStarted notification
void _handlePreChargeStarted(struct Framing *framing, struct Response *response, struct Charger *charger)
{
    printf("\"Pre Charge Started\" received\n");
    charger->is_stopped = 0;
}

// Handle the StartChargingRequested notification
void _handleRequestStartCharging(struct Framing *framing, struct Response *response, struct Charger *charger)
{
    printf("\"Start Charging Requested\" received\n");
    struct Schedule_profile *profile = v2gEvseParseStartChargingRequested(framing, response);
    printf("Schedule tuple ID: %d\n", profile->schedule_tuple_id);
    charger->is_stopped = 0;
	v2gEvseStartCharging(framing);
	free(profile);
}

// Handle the RequestStopCharging notification
void _handleRequestStopCharging(struct Framing *framing, struct Response *response, struct Charger *charger)
{
    printf("\"Request Stop Charging\" received\n");
    struct Notification *notification = v2gEvseParseStopChargingRequested(framing, response);
    printf("Timeout: %f\n", notification->timeout);
    printf("Renegotiation: %d\n", notification->renegotiation);
    charger->is_stopped = 1;
	v2gEvseStopCharging(framing);
	free(notification);
}

// Handle the WeldingDetectionStarted notification
void _handleWeldingDetectionStarted(struct Framing *framing, struct Response *response)
{
    printf("\"Welding Detection Started\" received\n");
}

// Handle the SessionStopped notification
void _handleSessionStopped(struct Framing *framing, struct Response *response, struct Charger *charger)
{
    charger->is_charging = 0;
    printf("\"Session stopped\" received\n");
    uint8_t closure_type = v2gEvseParseSessionStopped(framing, response);
    printf("Closure type: %f\n", closure_type);
    charger->is_stopped = 1;
}

// Handle the SessionError notification
void _handleSessionError(struct Framing *framing, struct Response *response, struct Charger *charger)
{
    printf("\"Session Error\" received\n");
    charger->is_charging = 0;
    uint8_t error_code = v2gEvseParseSessionError(framing, response);
    charger->is_stopped = 1;
    
    printf("Session error: %d: %s\n", error_code, error_messages[error_code]);
	v2gEvseStopCharging(framing);
}

// Handle the CertificateInstallationRequested notification
void _handleCertificateInstallationRequested(struct Framing *framing, struct Response *response)
{
	// !!! not implemented in original FreeV2G !!!
	// function is incomplete
    printf("\"Certificate Installation Requested\" received\n");
    struct EXI_request *exi = v2gEvseParseCertificateRequested(framing, response);
    printf("Timeout: %f\n", exi->timeout);
    printf("EXI request: %f\n", exi->data);
	free(exi);
}

// Handle the CertificateUpdateRequested notification
void _handleCertificateUpdateRequested(struct Framing *framing, struct Response *response)
{
	// !!! not implemented in original FreeV2G !!!
	// function is incomplete
    printf("\"Certificate Update Requested\" received\n");
    struct EXI_request *exi = v2gEvseParseCertificateRequested(framing, response);
    printf("Timeout: %f\n", exi->timeout);
    printf("EXI request: %f\n", exi->data);
	free(exi);
}

// Handle the MeteringReceiptStatus notification
void _handleMeteringReceiptStatus(struct Framing *framing, struct Response *response)
{   
    printf("\"Metering Receipt Status\" received\n");
    uint8_t status = v2gEvseParseMeteringReceiptStatus(framing, response);
    printf("Metering receipt status: %d\n", status);
}