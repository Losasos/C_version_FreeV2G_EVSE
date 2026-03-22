# WHITE-BEET Controll app EVSE Makefile

CC = C:/mingw64/bin/gcc.exe
BIN = wb.exe
OBJ = main.o npcap_lib.o wb_framing.o whitebeet.o evse.o

LIBS = -L"C:/mingw64/lib" "C:/dev/npcap/Lib/x64/wpcap.lib" "C:/dev/npcap/Lib/x64/Packet.lib" -lws2_32 -liphlpapi -User32

INCS = -I"C:/mingw64/include" -I"C:/dev/npcap/Include"
FLAGS = -s -static -static-libgcc -O2 $(INCS)

.PHONY : all clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LIBS)

main.o: main.c
	$(CC) -c main.c -o main.o $(FLAGS)

evse.o: evse.c
	$(CC) -c evse.c -o evse.o $(FLAGS)

whitebeet.o: whitebeet.c
	$(CC) -c whitebeet.c -o whitebeet.o $(FLAGS)

npcap_lib.o: npcap_lib.c
	$(CC) -c npcap_lib.c -o npcap_lib.o $(FLAGS)

wb_framing.o: wb_framing.c
	$(CC) -c wb_framing.c -o wb_framing.o $(FLAGS)

clean:
	del $(OBJ) $(BIN)