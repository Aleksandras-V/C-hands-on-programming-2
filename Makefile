DEBUG = -g
CC = /home/angel998/qnx710/host/linux/x86_64/usr/bin/qcc
LD = qcc

export QNX_HOST = /home/angel998/qnx710/host/linux/x86_64/
export QNX_TARGET = /home/angel998/qnx710/target/qnx7/

TARGET = -Vgcc_ntox86_64

CFLAGS += $(DEBUG) $(TARGET) -Wall
LDFLAGS+= $(DEBUG) $(TARGET)

clean:
	rm -f *.o ipc_receivefile ipc_sendfile

all: ipc_receivefile ipc_sendfile
