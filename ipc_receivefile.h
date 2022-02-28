#include <sys/mman.h>

#define MSG_RECV_NAME "IPC_MSG_RECEIVER"
#define SHMEM_SERVER_NAME "IPC_SHMEM_RECEIVER"
#define PIPE_RECV_NAME "IPC_PIPE_RECEIVER"


#define MAX_STRING_LEN    256


void display_help(void); // displays help message
void display_arg_error(void); // displays arguments error message
int ipc_receive_message(char* fileName);// receives a file using messages, returns status;
int ipc_receive_shm(char* fileName);// receives a file using shared memory, returns status;
int ipc_receive_pipe(char* fileName);// receives a file using pipe returns status;
int ipc_receive_queue(char* fileName);// receives a file using queue returns status;

void fileInit(char* file); // Prepares a file (overwrites if it isn't empty)
int writeFile(char* textmsg,char* file,int size); //Writes size characters of char* textmsg to the file specified.
int create_shared_memory(unsigned nbytes, int client_pid, void **ptr, shm_handle_t *handle);

typedef struct
{
	char string[MAX_STRING_LEN + 1]; //ptr to string we're sending to server, to make it
} ipc_msg_t;//easy, server assumes a max of 256 chars!!!!


/// SHARED MEMORY
// ask for a shared_mem_bytes sized object
typedef struct get_shmem_msg {
	uint16_t type;
	unsigned shared_mem_bytes;
} get_shmem_msg_t;

// reply that length bytes offset have been changed
typedef struct changed_shmem_resp {
	unsigned length;
} changed_shmem_resp_t;

