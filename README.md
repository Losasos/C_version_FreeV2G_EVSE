# C_version_FreeV2G_EVSE
This is a V2G aplication for WhiteBeet EVSE side
The application was translated from FreeV2G python Demo application (https://github.com/Sevenstax/FreeV2G) to C-code by CODICO GmbH (plc@codico.com)
#
This code is meant to be used as a template for quick start with WHITE-BEET-EI evaluation board or module on Windows (incl. Windows 11) using Ethernet interface.
The application uses NPCAP driver for low level access to Ethernet interface (NIC) on L2 OSI level.
#
The WHITE-BEET-EI control application (EVSE side) application could be run with or without parameters.
When run with parameters, please consider a fix order of parameters
    <iface> - Ethernet interface PCAP index (given by PCAP library)
    <mac> - MAC address of WHITE-BEET-EI module printed on the module.
            Actually MAC of QCA7005 chip, the MAC of STM32 is equal to MAC of QCA7005 "minus 2")
            Recommended to use npcap driver (please obtain from npcap.org)
E.g. wb_evse 6 C4930034A463
