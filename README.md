# C_version_FreeV2G_EVSE
This is FreeV2G rewritten in C (originally written in Python), which is a control application for WHITE-BEET-EI embedded module used in EV charging applications for communication (IEC/ISO15118 and DIN70121 standards) over Control Pilot (PLC/ powerline communication).

Project is rewritten in C utilizing Npcap library for Ethernet level communication (OSI L2).
Developed modules:
-	Ethernet connection and communication layer
-	Packing and unpacking Modem known Messages
-	EV Connection sequence and communication state machine (basic application layer)
