#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <getopt.h>
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


	const char* const short_options = "hm:f";


	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
	    { "messages",   0, NULL, 'm' },
		//{ "queue",     0, NULL, 'q' },
		//{ "pipe",     0, NULL, 'p' },
		//{ "shm",     0, NULL, 's' },
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
	att = name_attach(NULL, RECV_NAME, 0);
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
