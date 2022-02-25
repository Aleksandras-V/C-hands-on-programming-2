#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <getopt.h>
#include <semaphore.h>
#include "ipc_receivefile.h"
#include "ipc_sendfile.h"

/*typedef union
{
	ipc_msg_t msg;
} recv_buf_t;*/

typedef union
{
	uint16_t msg_type;
	//struct _pulse pulse;
	msg_header_t header;
} msg_buf_t;


int main(int argc, char *argv[]) {

	if (argc < 2){
		display_arg_error();
		exit(EXIT_FAILURE);
	}

	char* fileName=NULL;
	int next_option;


	const char* const short_options = "hmsp:f";


	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
	    { "messages",   0, NULL, 'm' },
		//{ "queue",     0, NULL, 'q' },
		{ "pipe",     0, NULL, 'p' },
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

	optind=0; // to reset the options index

	do {
	    next_option = getopt_long (argc, argv, short_options,long_options, NULL);
	    switch (next_option)
	    {
	    case 'h':   /* -h or --help */
	    	display_help();
	    	break;

	    case 'm':   /* -m or --messages */
	    	if (fileName == NULL){
	    		printf("ERROR: To receive an IPC message type:\nipc_receivefile --messages --file <path_to_dest/file>\nfor more informations:   ipc_receivefile --help    \n");
	    		exit(EXIT_FAILURE);
	    	}
	    	return ipc_receive_message(fileName);
	    case 's':   /* -s or --shm */
	    	if (fileName == NULL){
	    		printf("ERROR: To copy a file using shared memory type:\nipc_receivefile --shm --file <path_to_source/file>\nfor more informations:   ipc_receivefile --help    \n");
	    		exit(EXIT_FAILURE);
	    	}
	    	return ipc_receive_shm(fileName);
	    case 'p':   /* -s or --shm */
	    	if (fileName == NULL){
	    		printf("ERROR: To copy a file using pipe type:\nipc_receivefile --pipe --file <path_to_source/file>\nfor more informations:   ipc_receivefile --help    \n");
	    		exit(EXIT_FAILURE);
	    	}
	    	return ipc_receive_pipe(fileName);
	    case 'f':
	    	break;
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
    printf("\nThis program is made to receive a file sent by ipc_sendfile \n");
    printf("Commands:\n");
    printf("To display this message: ipc_receivefile --help \n");
    printf("To receive a file using messages: ipc_receivefile --messages --file <path_to_dest/file> \n");
    printf("To receive a file using shared memory: ipc_receivefile --shm --file <path_to_dest/file> \n");
    printf("To receive a file using pipe: ipc_receivefile --pipe --file <path_to_dest/file> \n");
}
void display_arg_error(void)
{
    printf("ERROR: This program must be started with correct command line arguments, for help:\n\n");
	printf("   ipc_receivefile --help    \n\n");
}



int ipc_receive_message(char* fileName)
{

	name_attach_t *att;
	int rcvid;
	msg_buf_t msg;
	int status;
	int messageStatus=1;


	// register our name so the client can find us
	att = name_attach(NULL, MSG_RECV_NAME, 0);
	if (att == NULL)
	{
		perror("name_attach()");
		exit(EXIT_FAILURE);
	}
	printf("Initializing file... \n");
	fileInit(fileName);
	while (1)
	{
		rcvid = MsgReceive(att->chid, &msg, sizeof(msg), NULL );
		if (rcvid == -1)
		{ //was there an error receiving msg?
			perror("MsgReceive"); //look up errno code and print
			exit(EXIT_FAILURE); //give up
		}
		else if (rcvid == 0)// we get a pulse
		{
			//todo
		}
		else //we get a message
		{
			printf("Got an IPC message \n");

			unsigned char* data;

			data = malloc(msg.header.data_size);
			if (data == NULL)
			{
				if (MsgError(rcvid, ENOMEM ) == -1)
				{
					perror("MsgError");
					exit(EXIT_FAILURE);
				}
			}
			else
			{

				iov_t siov[(msg.header.data_size/iov_block_size)+1];
				int last_part_size = msg.header.data_size%iov_block_size;

				int i;
				for (i=0;i<=(msg.header.data_size/iov_block_size);i++)
				{
					// preparing the buffer to receive the data
					if (i == (msg.header.data_size/iov_block_size)){
						SETIOV (&siov[i], &data [i*iov_block_size], last_part_size);
					}
					else {
						SETIOV (&siov[i], &data [i*iov_block_size], iov_block_size);
					}
				}
				printf("Receiving the message... \n");
				MsgReadv (rcvid, siov,(msg.header.data_size/iov_block_size)+1, sizeof (msg.header));

				printf("Writing to file... \n");
				messageStatus=writeFile(data,fileName,msg.header.data_size);//writing message to the file
				if (messageStatus != 0){
					printf("writeFile Error\n");
					exit(EXIT_FAILURE);
				}
				printf("Writing complete. \n");
			}
			status = MsgReply(rcvid, EOK,&messageStatus, sizeof(messageStatus));
			if (status == -1)
			{
				perror("MsgReply");
			}
			printf("Work done, exiting\n");
			return 0;
		}
	}
	return 0;
}

int ipc_receive_shm(char* fileName){

	typedef union
	{
		uint16_t type;
		struct _pulse pulse;
		get_shmem_msg_t get_shmem;
		changed_shmem_msg_t changed_shmem;
	} shmem_recv_buf_t;

	name_attach_t *att;
	int rcvid;
	shmem_recv_buf_t rbuf;
	int client_scoid = 0; // no client yet
	struct _msg_info msg_info;
	unsigned char *shmem_ptr;
	unsigned shmem_memory_size;
	get_shmem_resp_t get_resp;
	int status;
	sem_t * semPtr = NULL;				/* Pointer to semaphore */
	int copy_status = -1;

	fileInit(fileName);

	printf("Registering the receiver...\n");

	// register our name
	att = name_attach(NULL, SHMEM_SERVER_NAME, 0);

	if (att == NULL) {
		perror("name_attach");
		exit(EXIT_FAILURE);
	}

	while (1) {

		rcvid = MsgReceive(att->chid, &rbuf, sizeof(rbuf), &msg_info );

		if (rcvid == -1) {
			perror("MsgReceive");
			exit(EXIT_FAILURE);
		} else if (rcvid == 0) {
			if(rbuf.pulse.code == _PULSE_CODE_DISCONNECT) {
				// a client went away -- was it ours?
				if( rbuf.pulse.scoid == client_scoid ) {
					client_scoid = 0;
					munmap(shmem_ptr, shmem_memory_size);
				}
				ConnectDetach(rbuf.pulse.scoid);
			}
			if(rbuf.pulse.code == semaphore_ready) {
				// a client informed us that it created the semaphore -- was it ours?
				if( rbuf.pulse.scoid == client_scoid ) {
					client_scoid = 0;
				}
				printf("Setting the Semaphore\n");
				semPtr = sem_open("Semaphore",1);
				if (semPtr == SEM_FAILED){
					perror("sem_open");
					exit(EXIT_FAILURE);
				}

			}
		} else {
			//received a message
			switch (rbuf.type) {
			case GET_SHMEM_MSG_TYPE:
				if (semPtr == NULL){ // checking if a semaphore is set
					printf("ERROR : Received shared memory asking but no semaphore available.\n");
					exit(EXIT_FAILURE);
				}
				printf("Received shared memory asking.\n");
				shmem_memory_size = rbuf.get_shmem.shared_mem_bytes;

				status = create_shared_memory(shmem_memory_size,  msg_info.pid,  (void *)&shmem_ptr, &get_resp.mem_handle);
				if( status != 0 ) {
					MsgError(rcvid, errno);
					continue;
				}
				client_scoid = msg_info.scoid;

				printf("Shared memory created \n");
				//informing the client that shared memory is created !
				status = MsgReply(rcvid, EOK, &get_resp, sizeof(get_resp));
				if (status == -1) {
					perror("MsgReply");
					(void)MsgError(rcvid, errno);
				}
				break;

			case CHANGED_SHMEM_MSG_TYPE:
			{
				if(msg_info.scoid != client_scoid) {
					// only the current client may tell us to read the memory
					(void)MsgError(rcvid, EPERM);
					continue;
				}

				printf("Shared area updated !\n");

				const unsigned nbytes = rbuf.changed_shmem.length;
				if( (nbytes > shmem_memory_size)) {
					// oh no you don't
					MsgError(rcvid, EBADMSG);
					continue;
				}

				printf("Copying the memory...\n");
				while (sem_wait(semPtr) && (errno == EINTR)) { sleep(2);}
				copy_status=writeFile(shmem_ptr,fileName, nbytes);
				// Immediately giving access back to the memory
				status=sem_post(semPtr);
				if( status == -1 ) {
					perror("sem_post");
					exit(EXIT_FAILURE);
				}
				printf("Memory copied!\n");
				//informing the sender that the file is copied!
				status = MsgReply(rcvid, EOK, NULL, 0);
				if (status == -1) {
					// reply failed... try to unblock client with the error, just in case we still can
					perror("MsgReply");
					(void)MsgError(rcvid, errno);
				}
				printf("Unmapping the shared area and deleting the semaphore...\n");
				client_scoid = 0;
				munmap(shmem_ptr, shmem_memory_size);
				status = sem_unlink("Semaphore");
				if (status == -1) {
					perror("sem_unlink");
					exit(EXIT_FAILURE);
				}
				printf("Work done ! Exiting...\n");
				return copy_status;
			}

			default:
				// unknown message type, unblock client with an error
				if (MsgError(rcvid, ENOSYS) == -1) {
					perror("MsgError");
				}
				break;
			}

		}

	}

	return 1;
}

int ipc_receive_pipe(char* fileName){

	int status;
	int fd;
	int part_size;

	unsigned char* buffer;

	printf("Initializing file... \n");
	fileInit(fileName);

	buffer = malloc(pipe_size);
	if (buffer== NULL)
	{
		printf("ERROR: Malloc");
		exit(EXIT_FAILURE);
	}
	printf ("Connecting to Pipe... \n");
	do {
		status = access(pipe_name,F_OK);
		if (status == -1 ){
			sleep(2);
		}else{
			fd = open(pipe_name, O_RDONLY);
			if (fd == -1){
				perror ("open pipe");
				exit(EXIT_FAILURE);
			}
		}
	} while (status == -1);
	do{
		part_size=read(fd, buffer, pipe_size);
		if (part_size == -1)
		{
			perror("Pipe read");
			exit(EXIT_FAILURE);
		}
		status=writeFile(buffer,fileName,part_size);//writing message to the file
		if (status != 0){
			printf("writeFile Error\n");
			exit(EXIT_FAILURE);
		}
	}while(part_size != 0);

	free (buffer);
	printf("Writing complete, work done, exiting... \n");
	status=close (fd);
	if (status == -1)
	{
		perror("close()");
		exit(EXIT_FAILURE);
	}
	status=remove (pipe_name);
	if (status == -1)
	{
		perror("remove");
		exit(EXIT_FAILURE);
	}
	return 0;
}










int writeFile(unsigned char* textmsg,char* file, int size){
	FILE *fptr;
	int status;


	fptr = fopen(file,"ab");
	if (fptr == NULL){
		perror("fopen()");
		exit(EXIT_FAILURE);
	}
	fwrite(textmsg,size,1,fptr);

	status=fclose(fptr);
	if (status != 0){
		perror("fclose error");
		exit(EXIT_FAILURE);
	}
	return 0;
}

void fileInit(char* file){

	FILE *fptr;
	int size;
	int status;
	fptr = fopen(file,"r");
	if (fptr == NULL){
		perror("fopen()");
		exit(EXIT_FAILURE);
	}

    fseek (fptr, 0, SEEK_END);
    size = ftell(fptr);

    if (0 == size) { // file is empty
        printf("file is empty and ready\n");
    }else{			// file isn't empty
    	printf("file is not empty, overwriting...\n");
        status=fclose(fptr);
        if (status != 0){
        	perror("fclose error");
        	exit(EXIT_FAILURE);
        }
    	fptr = fopen(file,"w");
    	if (fptr == NULL){
    		perror("fopen()");
    		exit(EXIT_FAILURE);
    	}

    }
    status=fclose(fptr);
    if (status != 0){
    	perror("fclose error");
    	exit(EXIT_FAILURE);
    }
}

int create_shared_memory(unsigned nbytes, int client_pid, void **ptr, shm_handle_t *handle) {
	int fd;
	int ret;

	/* create an anonymous shared memory object */
	fd = shm_open(SHM_ANON, O_RDWR|O_CREAT, 0600);
	if( fd == -1 ) {
		perror("shm_open");
		return -1;
	}

	/* allocate the memory for the object */
	ret = ftruncate(fd, nbytes);
	if( ret == -1 ) {
		perror("ftruncate");
		close(fd);
		return -1;
	}

	/* get a local mapping to the object */
	*ptr = mmap(NULL, nbytes, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(*ptr == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return -1;
	}


	/* get a handle for the client to map the object */
	ret = shm_create_handle( fd, client_pid, O_RDWR, handle, 0);

	if( ret == -1 ) {
		perror("shmem_create_handle");
		close(fd);
		munmap( *ptr, nbytes );
		return -1;
	}

	/* we no longer need the fd, so cleanup */
	(void)close(fd);

	return 0;
}
