DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64

CFLAGS += $(DEBUG) $(TARGET) -Wall
LDFLAGS+= $(DEBUG) $(TARGET)

clean:
	rm -f *.o ipc_receivefile ipc_sendfile

all: ipc_receivefile ipc_sendfile
