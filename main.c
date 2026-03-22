//******************************************************************
// This is a V2G aplication for WhiteBeet EVSE side
// The application was translated from FreeV2G python Demo application (https://github.com/Sevenstax/FreeV2G) to C-code by CODICO GmbH (plc@codico.com)
//
// This code is meant to be used as a template for quick start with WHITE-BEET-EI evaluation board or module on Windows (incl. Windows 11) using Ethernet interface.
// The application uses NPCAP driver for low level access to Ethernet interface (NIC) on L2 OSI level.
//
// The WHITE-BEET-EI control application (EVSE side) application could be run with or without parameters.
// When run with parameters, please consider a fix order of parameters
//    <iface> - Ethernet interface PCAP index (given by PCAP library)
//    <mac> - MAC address of WHITE-BEET-EI module printed on the module.
//            Actually MAC of QCA7005 chip, the MAC of STM32 is equal to MAC of QCA7005 "minus 2")
//            Recommended to use npcap driver (please obtain from npcap.org)
// E.g. wb_evse 6 C4930034A463
//******************************************************************

// define for hardcoded MACs in main.c
//#define DEBUG
#define IFACE_PORT_NUMBER 5
#define WB_MAC {0xC4, 0x93, 0x00, 0x48, 0xB2, 0x0C}

// define for console info output in evse.c 
#define OUTPUT

#include "evse.h"

// FIDING INTERFACE MAC FOR WINDOWS
#define WORKING_BUFFER_SIZE 15000
#define NPCAP_DEVICE_NAME_OFFSET 12

void wb_test(struct Framing *framing);
int8_t str_to_mac(char *dst, char *src);
int main(int argc, char **argv);

// Initializes configuration structs and calls start_evse function
void wb_test(struct Framing *framing)
{
	// config
	struct Charger charger;
	struct Schedule schedule;
	struct EVSE_config evse_config;
	
	charger.timestamp_last_calc_u = 0;
	charger.timestamp_last_calc_i = 0;
	charger.evse_present_voltage = 0;
	charger.evse_present_current = 0;
	charger.evse_delta_i = 0;
	charger.evse_delta_u = 0;
	charger.evse_max_current = 0;
	charger.evse_min_current = 0;
	charger.evse_max_voltage = 0;
	charger.evse_min_voltage = 0;
	charger.evse_max_power = 0;
	charger.ev_max_current = 0;
	charger.ev_min_current = 0;
	charger.ev_max_power = 0;
	charger.ev_min_power = 0;
	charger.ev_max_voltage = 0;
	charger.ev_min_voltage = 0;
	charger.ev_target_voltage = 0;
	charger.ev_target_current = 0;
	charger.is_charging = 0;
	charger.is_stopped = 1;
	
    // Set regulation parameters of the charger
    charger.evse_delta_u = 0.5;
    charger.evse_delta_i = 0.05;

    // Set limitations of the charger
    charger.evse_max_voltage = 400;
    charger.evse_max_current = 100;
    charger.evse_max_power = 25000;
	
    // Set the schedule
    schedule.code = 0;
	schedule.energy_to_be_delivered = 0;
	schedule.schedule_tuple_count = 1;
    schedule.schedule_tuples[0].schedule_tuple_id = 1;
	schedule.schedule_tuples[0].schedule_entries_count = 3;
    
    schedule.schedule_tuples[0].schedule_entries[0].start = 0;
    schedule.schedule_tuples[0].schedule_entries[0].interval = 1800;
    schedule.schedule_tuples[0].schedule_entries[0].power = 25000;
    
    schedule.schedule_tuples[0].schedule_entries[1].start = 1800;
    schedule.schedule_tuples[0].schedule_entries[1].interval = 1800;
    schedule.schedule_tuples[0].schedule_entries[1].power = (int)(25000 * 0.75);
    
    schedule.schedule_tuples[0].schedule_entries[2].start = 3600;
    schedule.schedule_tuples[0].schedule_entries[2].interval = 82800;
    schedule.schedule_tuples[0].schedule_entries[2].power = (int)(25000 * 0.5);
	
	evse_config.evse_id_DIN_len = 15;
	memcpy(evse_config.evse_id_DIN, "+49*123*456*789\0", evse_config.evse_id_DIN_len+1);
	evse_config.evse_id_ISO_len = 15;
	memcpy(evse_config.evse_id_ISO, "DE*A23*E45B*78C\0", evse_config.evse_id_ISO_len+1);
	evse_config.protocol_count = 2;
	evse_config.protocol[0] = 0;
	evse_config.protocol[1] = 1;
	evse_config.payment_method_count = 1;
	evse_config.payment_method[0] = 0;
	evse_config.energy_transfer_mode_count = 4;
	evse_config.energy_transfer_mode[0] = 0;
	evse_config.energy_transfer_mode[1] = 1;
	evse_config.energy_transfer_mode[2] = 2;
	evse_config.energy_transfer_mode[3] = 3;
	evse_config.certificate_installation_support = 0;
	evse_config.certificate_update_support = 0;
    
	// enable mirror mode
    networkConfigSetPortMirrorState(framing, 1);

    // Start the charger
    start_evse(framing, &evse_config, &charger, &schedule);
	
    printf("EVSE loop finished\n");
    printf("Goodbye!\n");
}

int8_t str_to_mac(char *dst, char *src)
{
	for(int i=0; i<MAC_SIZE*2; i++)
	{
		if(isxdigit((unsigned char)src[i]) == 0)
		{
			printf ("Non hexodecimal charecter present or input is too short!\r\n");
			return -1;
		}
		src[i] = toupper(src[i]);
		
		if(src[i] > '9')
		{
			src[i] -= 7;
		}
	}
	
	for(int i=0; i<MAC_SIZE; i++)
	{
		dst[i] = (src[i*2]-48)<<4 | (src[i*2+1]-48);
	}
	
	return 0;
}

// ======= MAIN =======
int main(int argc, char **argv)
{
	// PCAP INITIALISATION =========================================
	char errbuf[PCAP_ERRBUF_SIZE];
	errbuf[1] = '\0';
	
	pcap_init(PCAP_CHAR_ENC_LOCAL, errbuf);
	
	// FINDING DEVICE -----------------------------------------
	pcap_if_t *current_device;
	
	// runtime device number choice (console input)
	#ifndef DEBUG
	if(argc == 3)
	{
		pcap_findalldevs(&current_device, errbuf);
		goto_device_number(&current_device, atoi(argv[1]));
	}
	else
	{
		current_device = findalldevs_by_uinput();
	}
	printf("Opening: %s %s \r\n", current_device->description, current_device->name);
	#endif
	
	// hardcoded device number choice
	#ifdef DEBUG
	pcap_findalldevs(&current_device, errbuf);
	goto_device_number(&current_device, IFACE_PORT_NUMBER);
	#endif
	
	// mac get mac of chosen iface
	u_char *iface_mac = get_device_mac(current_device->name);
	if(iface_mac == NULL)
	{
		printf("MAC not found!");
		return -1;
	}
	
	printf("Iface MAC: ");
	print_mac(iface_mac);
	
	// HANDLE OPENING ------------------------------------------
	pcap_t *pcap = pcap_open_live(current_device->name, MAX_FRAME_SIZE, PCAP_OPENFLAG_PROMISCUOUS, RECIVE_TIMEOUT, errbuf);
	
	// Free device list and remove dangling pointers
	pcap_freealldevs(current_device);
	current_device = NULL;
	
	if(pcap == NULL)
	{
		printf("Opening: FAIL \r\n", pcap);
		printf("Error: %s \r\n", errbuf);
		return 0;
	}
	else
	{
		printf("Opening: SUCCESS \r\n", pcap);
	}
	
	// Framing Setup (FILTER) -------------------------------------
	struct Framing framing;
	framing.pcap = pcap;
	framing.req_timeout = 1000;
	framing.response_buffer = NULL;
	framing.error = 0;
	framing.src = iface_mac;
	
	// Hardcoded WB MAC
	#ifdef DEBUG
	uint8_t wb_mac[MAC_SIZE] = WB_MAC;
	framing.dst = (uint8_t *)&wb_mac;
	#endif
	
	// Console input WB MAC 
	#ifndef DEBUG
	char wb_mac_str[MAC_SIZE*2+1];
	if(argc == 3)
	{
		memcpy(wb_mac_str, argv[2], MAC_SIZE*2);
	}
	else
	{
		printf ("Enter WB MAC (without ':'): ");
		if(scanf("%12s", wb_mac_str) == 0)
		{
			printf ("Charecter limit exided!\r\n");
			return -1;
		}
	}
	
	framing.dst = (uint8_t *)malloc(MAC_SIZE);
	if(str_to_mac(framing.dst, wb_mac_str) == -1)
	{
		return -1;
	}
	
	// subtruct 2 from QCA mac to get desired STM32 mac
	int_to_big_endian(framing.dst+3, big_endian_to_int(framing.dst+3, 3)-2, 3);
	
	printf ("STM32 MAC: ");
	hex_print(framing.dst, MAC_SIZE);
	printf ("\r\n");
	#endif
	
	struct bpf_program filter;
	char filter_str[100];
	//char *filter_str = "ether dst 000000000000 and ether src 000000000000\0";
	sprintf(filter_str, "ether dst %02X%02X%02X%02X%02X%02X and ether src %02X%02X%02X%02X%02X%02X\0",
		framing.src[0], framing.src[1], framing.src[2], framing.src[3], framing.src[4], framing.src[5],
		framing.dst[0], framing.dst[1], framing.dst[2], framing.dst[3], framing.dst[4], framing.dst[5]);
	
	printf("filter string: %s \r\n", filter_str);
	
	bpf_u_int32 netmask = 0;
	//netmask = ~netmask;
	printf("compiling filter ... \r\n");
	if(pcap_compile(pcap, &filter, filter_str, 1, netmask) < 0)
	{
		printf("error filter compiling gone wrong \r\n");
	}
	printf("applying filter ... \r\n");
	pcap_setfilter(pcap, &filter);
	
	// WB TEST  ============================================
	wb_test(&framing);
	pcap_close(pcap);
	printf("Communication CLOSED \r\n");
	
	//free_framing(&framing);

	printf("PRESS ENTER TO CLOSE THE PROGRAMM...");
	getchar();
	return 1;
}