/////////////////////////////////////////////////////////////////////////////////////////////////
//	File:		aesdsocket.c							       //
//	Author:		Michelle Christian						       //
//	ECEN 5713 AESD Assignment 5 part 1						       //
/////////////////////////////////////////////////////////////////////////////////////////////////

/*********************************************INCLUDES******************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <linux/fs.h>
#include <stdbool.h>
#include <sys/stat.h>

/*******************************************DEFINES*********************************************/
#define MAXSIZE 100
#define MYPORT "9000"  			// the port users will be connecting to
#define BACKLOG 10     			// how many pending connections queue will hold

/***************************************GLOBAL VARIABLE*****************************************/

int storefd, acceptfd, socketfd;	//aesdsocketdata file fd, client fd, server socket fd

//signal handler to handle SIGTERM and SIGINT signals
void sig_handler(int signo)
{
  if ((signo == SIGINT) || (signo == SIGTERM)){
    printf("Caught signal, exiting\n");
   
    close(storefd);						//close the file
    close(socketfd);						//close the server socket
    closelog();							//close log

    if(remove("/var/tmp/aesdsocketdata") == -1)                 //delete the file
    {   
        syslog(LOG_ERR,"file delete error!!");
    }
    
    exit(EXIT_SUCCESS);
  }
}

int main(int argc, char *argv[]){

	int status, bindret, listenret, recvret, writeret, readret, sendret;
	int sendsize=0;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in connection_addr;
	socklen_t addr_size;
	char buf[MAXSIZE];
	char* buf3; char* newline; 
	off_t ret;
	size_t buf2_size=0;
	int opt=1;
        bool daemon = false;	
	pid_t pid; 

	if(argc == 1){
		daemon = false;							//not running in daemon mode
	}

	else if(argc >2){							//more than required arguments
		syslog(LOG_ERR,"Please provide appropriate arguments!!");	
		return -1;
	}

	else if(argc == 2){							//if daemon argument provided
		if(strcmp(argv[1],"-d")==0)
			daemon = true;
	}

        if (signal(SIGINT, sig_handler) == SIG_ERR)				//call signal handler for SIGINT SIGTERM
              syslog(LOG_ERR,"\ncan't catch SIGINT\n");
        else if (signal(SIGTERM, sig_handler) == SIG_ERR)
              syslog(LOG_ERR,"\ncan't catch SIGINT\n");


	openlog(NULL,0,LOG_USER);
	memset(&hints, 0, sizeof(hints)); 		//make sure the struct is empty
	hints.ai_family 	= AF_INET;		//IPV4/IPV6
	hints.ai_socktype 	= SOCK_STREAM;		//TCP stream sockets
	hints.ai_flags		= AI_PASSIVE;		//fill IP automatically
	
	//getaddrinfo
 	if ((status = getaddrinfo(NULL, MYPORT, &hints, &res)) !=0){
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	    syslog(LOG_ERR, "error: getaddrinfo!! errno: %s", strerror(errno));
   	    return -1;
	}
	
	//get socketfd
	socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(socketfd == -1){
	    syslog(LOG_ERR,"error: socket!! errno: %s", strerror(errno));
	    return -1;
	}

        // Forcefully attaching socket to the port 
   	if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    	{
        	perror("setsockopt");
        	exit(EXIT_FAILURE);
    	}

	//bind the address and check return value
        bindret = bind(socketfd, res->ai_addr , res->ai_addrlen);
	if(bindret == -1){
	    syslog(LOG_ERR, "error: bind!! errno: %s", strerror(errno));
	    return -1;
	}

	//free the malloc
	freeaddrinfo(res);
	
	
        // Forcefully attaching socket to the port
        if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
        {
                perror("setsockopt");
                exit(EXIT_FAILURE);
        }

	//if daemon argument provided fork the process
        if(daemon){
                syslog(LOG_INFO, "Running in daemon mode!!");

                pid = fork();
                if (pid == -1)
                        return -1;
                else if (pid != 0)
                        exit (EXIT_SUCCESS);
                /* create new session and process group */
                if (setsid () == -1)
                        return -1;
                /* set the working directory to the root directory */
                if (chdir ("/") == -1)
                        return -1;

                /* redirect fd's 0,1,2 to /dev/null */
                open ("/dev/null", O_RDWR); /* stdin */
                dup (0); /* stdout */
                dup (0); /* stderror */
        }

	//listen for a connection
	listenret = listen(socketfd, BACKLOG);
	if(listenret == -1){
	    syslog(LOG_ERR, "error: listen!! errno: %s", strerror(errno));
	    return -1;
	}

        //open file
        storefd = open("/var/tmp/aesdsocketdata", O_CREAT | O_APPEND | O_RDWR, 0666);
        if(storefd == -1){
               syslog(LOG_ERR, "error: file open/creation error!! errno:%s", strerror(errno));
               return -1;
         }



       addr_size = sizeof connection_addr;

	while(1){
		
                //accept for a connection
                acceptfd = accept(socketfd, (struct sockaddr*)&connection_addr, &addr_size);
                if(acceptfd == -1){
                    syslog(LOG_ERR, "error: accept!! errno:%s", strerror(errno));
                    return -1;
                }

                syslog(LOG_INFO,"Accepted connection from %d", acceptfd);
		
		//clear the buffer for input
	        memset(buf, '\0', sizeof(buf));
		char* buf2 = (char*)malloc(MAXSIZE*sizeof(char));
		int mallocsize = MAXSIZE;
		char* newptr = NULL;

		do{
				//receive the bytes
				recvret = recv(acceptfd, buf, sizeof(buf), 0);
				if(recvret == -1){
					syslog(LOG_ERR, "errno: %s", strerror(errno));
				}
				else{
					if((mallocsize-buf2_size) < recvret){
						mallocsize += recvret;
						newptr = (char*)realloc(buf2, mallocsize* sizeof(char)); //realloc if the recived bytes are more than the size of buffer
						if(newptr!=NULL){
							buf2 = newptr;
						}
					}
					memcpy(&buf2[buf2_size], buf, recvret);				//copy contents to another buffer so later can append bigger data files
				}

			buf2_size+=recvret;
			newline = strchr(buf, '\n');	//check for newline character
		}while(newline == NULL); 

		//write into the file
                writeret = write(storefd, buf2, buf2_size);
                       if(writeret == -1){
                                syslog(LOG_ERR, "Write error!!");
                       }


		//read from the file and send
		ret = lseek(storefd, 0, SEEK_SET);
			if(ret == (off_t) -1){
				syslog(LOG_ERR, "lseek error!!");
			}

		
		sendsize+=buf2_size;	
		buf3 = (char*)malloc(sendsize*sizeof(char));

		//read from /var/tmp/aesdsocketdata
		readret = read(storefd, buf3, sendsize);
			if(readret == -1){
				syslog(LOG_ERR,"read error!!");
			}	
	
		//send data
		sendret = send(acceptfd, buf3, sendsize, 0);
			if(sendret == -1){
				syslog(LOG_ERR,"send error!!");
			}

	   //free the malloced buffers to avoid memory leak	
	   free(buf2);
   	   free(buf3);	   

	   //close the client fd
	   close(acceptfd);
	   syslog(LOG_INFO, "Closed connection from %d", acceptfd);

	   //reinitialize the variables for new line input
	      buf2_size =0;
	      recvret=0;
	      readret=0;
	}

	//close the file 
	close(storefd);
	close(socketfd);

	closelog();

	return 0;
}
