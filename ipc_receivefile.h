
#define RECV_NAME "IPC_RECEIVER"


#define MAX_STRING_LEN    256


void display_help(void); // displays help message
void display_arg_error(void); // displays arguments error message
int ipc_receive_message(char* fileName);// receives a file using messages, returns status;

void fileInit(char* file); // Prepares a file (overwrites if it isn't empty)
int writeFile(unsigned char* textmsg,char* file,int size); //Writes size characters of char* textmsg to the file specified.

typedef struct
{
	unsigned char string[MAX_STRING_LEN + 1]; //ptr to string we're sending to server, to make it
} ipc_msg_t;//easy, server assumes a max of 256 chars!!!!
