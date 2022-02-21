int ipc_send_message(char* fileName);
unsigned char* readFile(char *File); // Allocates and returns the address to the binary data of a file;
int lineNumberFile(char* File); // return the number of lines of a file.
int filesize(char* filename);


#define IOV_MSG_TYPE (_IO_MAX + 1)
#define iov_block_size 4096

typedef struct
{
	uint16_t msg_type;
	unsigned data_size;
} msg_header_t;
