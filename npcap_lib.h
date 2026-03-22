#ifndef NPCAP_LIB_H
#define NPCAP_LIB_H

#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pcap.h>

// FIDING INTERFACE MAC FOR WINDOWS
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#define WORKING_BUFFER_SIZE 15000
#define NPCAP_DEVICE_NAME_OFFSET 12

// SETTINGS
#define MAX_FRAME_SIZE 65536
#define RECIVE_TIMEOUT 10 // ms
#define MAX_ETH_FRAME_SIZE 1600 // 1600 to compensate the variativity 15xx
#define MAC_SIZE 6
#define ETH_TYPE 0x6003
#define ETH_TYPE_SIZE 2

// Function deffinition
u_char* mac_to_str(u_char mac[MAC_SIZE]);
void binary_print(int number, int size);
void hex_print(uint8_t *data, int length);
void print_hex(const u_char* bytes, int len);
void print_mac(const u_char* mac);

unsigned int build_frame(uint8_t **frame, uint8_t dst[], uint8_t src[], uint16_t eth_type, uint8_t payload[], unsigned int payload_len);
int filter_devices(pcap_if_t *devices, int ex_mask, char* ex_name);
void print_alldevs(pcap_if_t *alldevices);
void goto_device_number(pcap_if_t **alldevices, unsigned int device_number);
pcap_if_t* findalldevs_by_uinput();
u_char* get_device_mac(const char* devs_name);
int8_t pcap_handle_status(int8_t response_status, pcap_t *pcap);

#endif