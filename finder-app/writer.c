/*	AESD- Assignment2		      *							
 *	C implementation of writer.sh script  *
 *	Author: Michelle Christian	      */

/**********************************************************************************************/
/*	Functionality: if thefile exists it overwrites the contents of the file with the string
 *			writestr, if not the file is created and writes the content into it
**********************************************************************************************/

/****************************************INCLUDES*********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

/**************************************FUNCTION DEFINITION***********************************/


int main(int argc, char *argv[]){


        int fd;
        char* writefile;
        char *writestr;
        ssize_t ret;
        int len;


	//Setting up syslog logging for utility using the LOG_USER facility
	openlog(NULL,0, LOG_USER);


	//checking the conditions that all the arguments have been provided including filename and the string to be written to the file
	if(argc <3 || argc>3){
		syslog(LOG_ERR,"You have not provided all the arguments. The number of arguments provided are: %d", argc);
		printf("All the required arguments have not provided!\n");
		printf("First argument: Path including the filename\nSecond argument: String to be written in the file\n");
		exit(1);
	}

        writefile = argv[1];
        writestr = argv[2];
        len = strlen(writestr);

	//opening the file with user read and write permissions and if it does not exist create it
	fd = open(writefile, O_RDWR| O_CREAT| O_TRUNC, S_IRUSR| S_IWUSR);

	//If the file could not be opened if it exists or if it could not be created, generate error message and exit with status 1
	if(fd == -1){
			syslog(LOG_ERR,"File could not be created\n");
			exit(1);
	}

	syslog(LOG_DEBUG, "File successfully opened!\n");		
	syslog(LOG_DEBUG, "Writing %s to %s",writestr, writefile);

	//using a while loop to avoid partial writes
		while (len != 0 && (ret = write (fd, writestr, len)) != 0) {
			if (ret == -1) {
					if (errno == EINTR)
					continue;
				printf("File could not be created");
				syslog(LOG_ERR, "Could not write to the file");
				perror ("write");
				break;
			}
     		len -= ret;
		writestr += ret;
		}
	
	
	//checking if the file has been written successfully or not 
	if(len == 0)
		syslog(LOG_DEBUG, "File successfully written!\n");
	else
		syslog(LOG_DEBUG, "Could not write into file");

}

//
