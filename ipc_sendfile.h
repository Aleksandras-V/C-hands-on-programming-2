#include <sys/mman.h>


int ipc_send_message(char* fileName);
int ipc_send_shm(char* fileName);
int ipc_send_pipe(char* fileName);
unsigned char* readFile(char *File); // Allocates and returns the address to the binary data of a file;

int filesize(char* filename);


#define IOV_MSG_TYPE (_IO_MAX + 1)
#define GET_SHMEM_MSG_TYPE (_IO_MAX + 2)
#define CHANGED_SHMEM_MSG_TYPE (_IO_MAX+3)
//#define RELEASE_SHMEM_MSG_TYPE (_IO_MAX+4)
#define PIPE_FD_SEND (_IO_MAX+5)
#define iov_block_size 4096
#define semaphore_ready (_PULSE_CODE_MINAVAIL)
#define pipe_size 4096

typedef struct
{
	uint16_t msg_type;
	unsigned data_size;
} msg_header_t;

/// SHARED MEMORY
// response provides a handle to that object
typedef struct get_shmem_resp {
	shm_handle_t mem_handle;
} get_shmem_resp_t;

// inform the server that length bytes starting at offset have been changed
typedef struct changed_shmem_msg {
	uint16_t type;
	unsigned length;
} changed_shmem_msg_t;

/// PIPE
#define pipe_name ("/tmp/pipe")
