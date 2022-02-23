#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <getopt.h>
#include <sys/netmgr.h>
#include <semaphore.h>
#include "ipc_receivefile.h"
#include "ipc_sendfile.h"

int main(int argc, char *argv[]) {

	if (argc < 2){
		display_arg_error();
		exit(EXIT_FAILURE);
	}

	char* fileName=NULL;
	int next_option;


	const char* const short_options = "hms:f";


	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
	    { "messages",   0, NULL, 'm' },
		//{ "queue",     0, NULL, 'q' },
		//{ "pipe",     0, NULL, 'p' },
		{ "shm",     0, NULL, 's' },
		{ "file",     1, NULL, 'f' },
		{ NULL,       0, NULL, 0   }   /* Required at end of array.  */
	};

	int next_option_fileName;
	do { // loop for finding the fileName
		next_option_fileName = getopt_long (argc, argv, short_options,long_options, NULL);
		switch (next_option_fileName){
		case 'f': /* -f or --file */
			/* This option takes an argument, the name of the output file.  */
		    fileName = optarg;
		    break;
		}
	}
	while (next_option_fileName != -1 && fileName == NULL);

	optind=0; // to reset the options index for our new loop

	do {
	    next_option = getopt_long (argc, argv, short_options,long_options, NULL);
	    switch (next_option)
	    {
	    case 'h':   /* -h or --help */
	    	display_help();
	    	break;

	    case 'm':   /* -m or --messages */
	    	if (fileName == NULL){
	    		printf("ERROR: To send an IPC message type:\nipc_sendfile --messages --file <path_to_source/file>\nfor more informations:   ipc_sendfile --help    \n");
	    		exit(EXIT_FAILURE);
	    	}
	    	return ipc_send_message(fileName);
	    case 's':   /* -s or --shm */
	    	if (fileName == NULL){
	    		printf("ERROR: To copy a file using shared memory type:\nipc_sendfile --shm --file <path_to_source/file>\nfor more informations:   ipc_sendfile --help    \n");
	    		exit(EXIT_FAILURE);
	    	}
	    	return ipc_send_shm(fileName);
	    	//return 1;

	    case '?':   /* The user specified an invalid option.  */
	    	display_arg_error();
	    	exit(EXIT_FAILURE);

	    default:    /* Something else: unexpected.  */
	    	exit(EXIT_FAILURE);
	    }
	  }
	  while (next_option != -1);

   return 0;
}
void display_help(void)
{
    printf("\nThis program is made to send a file to ipc_receivefile \n");
    printf("Commands:\n");
    printf("To display this message: ipc_sendfile --help \n");
    printf("To send a file using messages: ipc_sendfile --messages --file <path_to_source/file> \n");
    printf("To copy a file using shared memory type:\nipc_sendfile --shm --file <path_to_source/file>\n");
}
void display_arg_error(void)
{
    printf("ERROR: This program must be started with commandline arguments, for example:\n\n");
	printf("   ipc_sendfile --help    \n\n");
}
int ipc_send_message(char* fileName){

	printf("Preparing to receive a message:\n");

	int coid; //Connection ID to server
	int status;
	int incoming_status;
	int fileSize;

	coid =-1;

	while (coid == -1){
		printf("Connecting to message receiver... \n");
		coid = name_open(MSG_RECV_NAME, 0);
		if (coid == -1){
			sleep(2);
		}
	}
	printf("Connected to the receiver \n");

	fileSize=filesize(fileName);

	int i;

	printf("file size: %d\n",fileSize);

	msg_header_t header;
	header.data_size=fileSize;
	header.msg_type=IOV_MSG_TYPE;

	int last_part_size = fileSize%iov_block_size;
	iov_t siov[(fileSize/iov_block_size)+2];

    unsigned char* buffer=readFile(fileName); //getting the data into the buffer

	SETIOV (&siov[0], &header, sizeof header); //setting header IOV

	for(i=1;i<=fileSize/iov_block_size+1;i++){ //setting data IOVs
		if (i == fileSize/iov_block_size+1){ //in case it is the last IOV
			SETIOV (&siov[i], &buffer[(i-1)*iov_block_size], last_part_size);
		} else { //else
			SETIOV (&siov[i], &buffer[(i-1)*iov_block_size], iov_block_size);
		}
	}
	//sending the prepared data
	printf("Sending data...\n");
	status = MsgSendvs(coid, siov, (fileSize/iov_block_size)+2,&incoming_status,sizeof(incoming_status));
	free (buffer);
	if (status == -1)
	{ //was there an error sending to server?
		perror("MsgSendvs");
		exit(EXIT_FAILURE);
	}
	if (incoming_status==0)
	{
		printf("data copied successfully \n");
	}
	else
	{
		printf("data copy failed \n");
		exit(EXIT_FAILURE);
	}
	printf("Work done, exiting...\n");

	return 0;
}

int ipc_send_shm(char* fileName){
	int coid=-1;
	int status;
	int mem_fd;
	char *mem_ptr;

	get_shmem_msg_t get_msg;
	get_shmem_resp_t get_resp;

	changed_shmem_msg_t changed_msg;

	sem_t* semPtr;	/* Pointer to semaphore */

	unsigned char* content;


	void* gatekeeper(void*);

	status=pthread_setname_np(pthread_self() ,"shm_file_exchange");
	if (status != 0){
		perror("pthread_setname_np");
		exit(EXIT_FAILURE);
	}

	int fileSize = filesize(fileName);

	content = readFile(fileName);

	/* find our server */
	while (coid == -1){
		printf("Connecting to shared memory receiver... \n");
		coid = name_open(SHMEM_SERVER_NAME, 0);
		if (coid == -1){
			sleep(2);
		}
	}

	printf("Creating semaphore to protect shared memory... \n");

	semPtr= sem_open("Semaphore",O_CREAT,O_EXCL,S_IRWXU,1);
	if (semPtr == SEM_FAILED){
		perror ("sem_open");
		exit(EXIT_FAILURE);
	}
	status = MsgSendPulse(coid, -1, semaphore_ready, 0);
	if (status == -1)
	{
		perror("MsgSendPulse");
	}

	get_msg.type = GET_SHMEM_MSG_TYPE;
	get_msg.shared_mem_bytes = fileSize;

	printf("Asking for creation of a shared area... \n");
	status = MsgSend(coid, &get_msg, sizeof(get_msg), &get_resp, sizeof(get_resp));
	if (status == -1)
	{
		perror("Get shmem MsgSend");
		exit(EXIT_FAILURE);
	}

	mem_fd = shm_open_handle(get_resp.mem_handle, O_RDWR);
	if( mem_fd == -1 ) {
		perror("shm_open_handle");
		exit(EXIT_FAILURE);
	}

	mem_ptr = mmap(NULL, fileSize, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0);
	if(mem_ptr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	/* once mapped, we don't need the fd anymore */
	close(mem_fd);

	printf("Writing in the shared area... \n");
	/* put some data into the shared memory object */
	while (sem_wait(semPtr) && (errno == EINTR)) { sleep(2);}
	// Semaphore was decremented
	memcpy(mem_ptr,content,fileSize);
	//Immediately giving access back to the memory
	status=sem_post(semPtr);
	if( status == -1 ) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}
	printf("Done! \n");

	changed_msg.type = CHANGED_SHMEM_MSG_TYPE;
	changed_msg.length = fileSize;

	// informing the receiver that the data in the shared memory has been modified

	printf("Informing the receiver! \n");
	status = MsgSend( coid, &changed_msg, sizeof(changed_msg), NULL, 0);
	if( status == -1 ) {
		perror("Change shmem MsgSend");
		exit(EXIT_FAILURE);
	}
	printf("Unmaping the shared area! \n");
	(void)munmap(mem_ptr, fileSize);
	printf("Work done, exiting! \n");

	return 0;
}








unsigned char* readFile(char *File){

	int fd;
    int size_read;
    int fileSize= filesize(File);
    unsigned char* content = malloc(fileSize);

    fd = open( File, O_RDONLY );
    if (fd == -1){
    	perror("open");
    	exit(EXIT_FAILURE);
    }

    size_read = read( fd, content,fileSize);

    if( size_read == -1 ) {
        perror( "Error reading myfile" );
        exit (EXIT_FAILURE);
    }
    close (fd);

    return content;
}
int filesize(char* filename){

	FILE *fptr;
	int status;
	int size;

	fptr=fopen(filename,"rb");
	if (fptr == NULL){
		perror("fopen()");
		exit(EXIT_FAILURE);
	}

	fseek(fptr, 0L, SEEK_END);// going to the end of the file
	size = ftell(fptr);


	status=fclose(fptr);
	if (status != 0){
		perror("fclose error");
		exit(EXIT_FAILURE);
	}

	return size;
}
