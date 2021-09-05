//AESD- Assignment2
//C implementation of writer.sh script
//Author: Michelle Christian

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

/* params	: two strings writefile and writestr
 * brief	: if the directory and file exists it overwrites the contents 
 * 		  of the file with the string writestr, if not it created the 
 * 		  path and file and writes the content into it
 * return-type	: void*/
int main(int argc, char *argv[]){

	if(argc <3){
		syslog(LOG_ERR,"Required arguments have not been provided correctly!");
		syslog(LOG_USER,"First argument: The path including filename\nSecond argument: The string to be written");
		perror("Arguments not provided!");
		exit(1);
	}

	int fd;
	char* writefile = argv[1];

	fd = open(writefile, O_RDWR| O_CREAT| O_TRUNC, S_IRUSR| S_IWUSR);

	if(fd == -1){
			syslog(LOG_ERR,"File could not be created\n");
			exit(1);
	}

	syslog(LOG_DEBUG, "File successfully opened!\n");

	char *buf = argv[2];

	ssize_t ret;
	int len = strlen(buf);
		
		while (len != 0 && (ret = write (fd, buf, len)) != 0) {
			if (ret == -1) {
					if (errno == EINTR)
					continue;
				perror ("write");
				break;
			}
     		len -= ret;
		buf += ret;
		}
	
	
	if(len == 0)
		syslog(LOG_DEBUG, "File successfully written!\n");
	else
		syslog(LOG_DEBUG, "Could not write into file");

}

