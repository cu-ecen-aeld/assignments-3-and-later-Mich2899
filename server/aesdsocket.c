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
#include <netinet/in.h>
#include <arpa/inet.h>
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

int storefd, acceptfd, socketfd, signalflag=0;	//aesdsocketdata file fd, client fd, server socket fd

//signal handler to handle SIGTERM and SIGINT signals
void sig_handler(int signo)
{
  if ((signo == SIGINT) || (signo == SIGTERM)){
    printf("Caught signal, exiting\n");

    if(shutdown(socketfd, SHUT_RDWR) == -1){
	perror("Shutdown error!!");
        syslog(LOG_ERR, "Client Shutdown error!!");
    }

	signalflag=1;

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

	openlog(NULL,0,LOG_USER);
	
	        if (signal(SIGINT, sig_handler) == SIG_ERR)				//call signal handler for SIGINT SIGTERM
              syslog(LOG_ERR,"\ncan't catch SIGINT\n");
        else if (signal(SIGTERM, sig_handler) == SIG_ERR)
              syslog(LOG_ERR,"\ncan't catch SIGINT\n");


	
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



       addr_size = sizeof connection_addr;

	while(!signalflag){
		
                //accept for a connection
                acceptfd = accept(socketfd, (struct sockaddr*)&connection_addr, &addr_size);
                
                if(signalflag)
                	break;
                	
                if(acceptfd == -1){
                    syslog(LOG_ERR, "error: accept!! errno:%s", strerror(errno));
                    return -1;
                }
                
		char *IP = inet_ntoa(connection_addr.sin_addr);
		syslog(LOG_DEBUG, "Accepted connection from %s\n", IP);
		
		//clear the buffer for input
	        memset(buf, '\0', sizeof(buf));
		char* buf2 = (char*)malloc(MAXSIZE*sizeof(char));
		if(buf2 == NULL){
			syslog(LOG_ERR, "read buffer malloc failed!!");
			break;
		}

		int mallocsize = MAXSIZE, reallocsize = MAXSIZE;
		char* newptr = NULL;
		int setback=0, pos=0;
		
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


		pos = lseek(storefd, 0, SEEK_CUR);
                ret = lseek(storefd, 0, SEEK_SET);
                        if(ret == (off_t) -1){
                                syslog(LOG_ERR, "lseek error!!");
                        }


		while(setback!= pos){
			sendsize=0;
			
			buf3 = (char*)malloc(MAXSIZE*sizeof(char));
			if(buf3 == NULL){
				syslog(LOG_ERR,"writebuffer malloc failed!!");
				break;
			}

	                //clear the buffer for input
        	        memset(buf3, '\0', MAXSIZE);

			do{
			
		               readret = read(storefd, buf3+sendsize, sizeof(char));
	                        if(readret == -1){
	                                syslog(LOG_ERR,"read error!!");
	                        }       
                        	
				if(reallocsize - sendsize < pos)
				{
					reallocsize += pos;
					buf3 = (char*)realloc(buf3, reallocsize*sizeof(char));
				}

				sendsize += readret;
 
			}while(newline==NULL);

			setback+=sendsize;
			
			//send data
           		sendret = send(acceptfd, buf3, sendsize, 0);
                        if(sendret == -1){
                                syslog(LOG_ERR,"send error!!");
                        }
	
     
			free(buf3);
  

		}

	   //free the malloced buffers to avoid memory leak	
	   free(buf2);
 	   close(acceptfd);	
   	   syslog(LOG_INFO,"Closed connection from %s",IP);	   

	   //reinitialize the variables for new line input
	      buf2_size =0;
	      recvret=0;
	      readret=0;
	      setback=0;
	      pos=0;
	}

	//close the file 
	close(storefd);
	close(socketfd);
	close(acceptfd);
	closelog();
	
	    if(remove("/var/tmp/aesdsocketdata") == -1)                 //delete the file
    	    {   
        	syslog(LOG_ERR,"file delete error!!");
    	    }    

	return 0;
}
