////////////////////////////////
// this file contains functions for setting up eth interface and finding its mac address
// for use with NPCAP

#include "./npcap_lib.h"

// FUNCTIONS =====================================================================================================

u_char* mac_to_str(u_char mac[MAC_SIZE])
{
	// 12 bytes for every symbol of 6 bytes mac address 
	// plus 1 byte for string termination sign
	u_char* str = (u_char*) malloc(13);
	sprintf(str, "%02X%02X%02X%02X%02X%02X\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return str;
}

// Prints int value as binary
void binary_print(int number, int size)
{
	size_t mask = 1;
	size *= 8;
	mask = mask << size-1;
	for(uint8_t i=0; i<size; i++)
	{
		mask = mask >> 1;
		if(number & mask)
		{
			printf("1");
		}
		else
		{
			printf("0");
		}
	}
}

// Prints an array of binary data in a hex format
void hex_print(uint8_t *data, int length)
{
	for(int i=0; i<length; i++)
	{
		printf("%02X", data[i]);
	}
}

// Builds Ethernet frame
unsigned int build_frame(uint8_t **frame, uint8_t dst[], uint8_t src[], uint16_t eth_type, uint8_t payload[], unsigned int payload_len)
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
	
	memcpy(++iterator, payload, payload_len);
	iterator += payload_len;
	frame_size = iterator - *frame;
	
	return frame_size;
}

// Excludes (black lists) mask and name
int filter_devices(pcap_if_t *devices, int ex_mask, char* ex_name)
{
	if(devices->flags & ex_mask)
	{
		return 0;
	}
	
	if(strstr(devices->description, ex_name) != NULL)
	{
		return 0;
	}
	
	return 1;
}

// Prints device description and indexes
void print_alldevs(pcap_if_t *alldevices)
{
	unsigned int device_number = 0;
	
	printf("-DEVICE LIST- \r\n");
	while(alldevices != NULL)
	{
		device_number++;
		
		if(filter_devices(alldevices, (PCAP_IF_LOOPBACK | PCAP_IF_WIRELESS), "WAN Miniport"))
		{
			// TODO friendly names (GetAdaptersAddresses winapi)
			printf("%d - %s \r\n", device_number ,alldevices->description);
		}
		alldevices = alldevices->next;
	}
}

// Moves linked list pointer to desired index
void goto_device_number(pcap_if_t **alldevices, unsigned int device_number)
{
	for(int i=1; i<device_number; i++)
	{
		*alldevices = (*alldevices)->next;
	}
}

// Prints devices, recives index from users input und returns chosen device struct
pcap_if_t* findalldevs_by_uinput()
{
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_if_t *alldevices;
	int device_number;
	
	pcap_findalldevs(&alldevices, errbuf);
	
	print_alldevs(alldevices);
	
	printf("Choose device: ");
	scanf("%d", &device_number);
	printf("\r\n");
	
	goto_device_number(&alldevices, device_number);
	
	return alldevices;
}

// Prints array of bytes in hexodecimal format
void print_hex(const u_char* bytes, int len)
{
	for(int i=0; i<len; i++)
	{
		printf("%X", bytes[i]);
	}
}

// Prints array of bytes of MAC address size in MAC format (:)
void print_mac(const u_char* mac)
{
	// MAC address is 6 bytes long
	for(int i=0; i<MAC_SIZE; i++)
	{
		printf("%X:", mac[i]);
	}
	printf("\b \r\n");
}

// WINDOWS ONLY
// Finds MAC address by device name
u_char* get_device_mac(const char* device_name)
{
	// TODO error check
	ULONG outBufLen = WORKING_BUFFER_SIZE;
	DWORD dwRetVal = 0;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	u_char *device_mac = NULL;
	device_name += NPCAP_DEVICE_NAME_OFFSET;
	
	pAddresses = (IP_ADAPTER_ADDRESSES*) MALLOC(outBufLen);
	GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen);
	PIP_ADAPTER_ADDRESSES initial_pAddresses = pAddresses;

	while(pAddresses != NULL)
	{
		if(strcmp(pAddresses->AdapterName, device_name) == 0)
		{
			device_mac = (u_char*)malloc(MAC_SIZE);
			memcpy(device_mac, pAddresses->PhysicalAddress, MAC_SIZE);
			break;
		}
		pAddresses = pAddresses->Next;
	}
	FREE(initial_pAddresses);
	return device_mac;
}

// Handels status of frame sending
int8_t pcap_handle_status(int8_t response_status, pcap_t *pcap)
{
	if(response_status == 1)
	{
		// packet recieved
		return 1;
	}
	
	if(response_status == 0)
	{
		//printf("No frame recived\n");
		return 0;
	}
	
	if(response_status == PCAP_ERROR_NOT_ACTIVATED)
	{
		printf("Handle not Active\n");
		return -1;
	}
	
	if(response_status == PCAP_ERROR)
	{
		printf(pcap_geterr(pcap));
		return -1;
	}
	
	return -1;
}