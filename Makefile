DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64

CFLAGS += $(DEBUG) $(TARGET) -Wall
LDFLAGS+= $(DEBUG) $(TARGET)

BINS = ipc_sendfile ipc_receivefile

all: $(BINS)

ipc_sendfile: ipc_sendfile.c
ipc_receivefile: ipc_receivefile.c

clean:
	rm -f *.o $(BINS)
