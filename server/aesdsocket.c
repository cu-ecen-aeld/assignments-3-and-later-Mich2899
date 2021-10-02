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

#define MAXSIZE 100
#define MYPORT "9000"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold

int storefd, acceptfd;

void sig_handler(int signo)
{
  if (signo == SIGINT || signo == SIGTERM){
    printf("Caught signal, exiting\n");
    remove("/var/tmp/aesdsocketdata");
    close(acceptfd);
    close(storefd);
    exit(EXIT_SUCCESS);
  }
}

int main(void){

	int status, socketfd, bindret, listenret, recvret, writeret, readret, sendret, flag=0;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in connection_addr;
	socklen_t addr_size;
//	char buf[MAXSIZE], buf2[MAXSIZE];
	char* buf; char* buf2;
	off_t ret;

	buf = (char*)malloc(100*sizeof(char));
	buf2 = (char*)malloc(100*sizeof(char));

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
	
	//bind the address and check return value
        bindret = bind(socketfd, res->ai_addr , res->ai_addrlen);
	if(bindret == -1){
	    syslog(LOG_ERR, "error: bind!! errno: %s", strerror(errno));
	    return -1;
	}

	//free the malloc
	freeaddrinfo(res);
	
	//listen for a connection
	listenret = listen(socketfd, BACKLOG);
	if(listenret == -1){
	    syslog(LOG_ERR, "error: listen!! errno: %s", strerror(errno));
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

		//recieve data
			
	        storefd = open("/var/tmp/aesdsocketdata", O_CREAT | O_APPEND | O_RDWR, 0666);
	        if(storefd == -1){
	            syslog(LOG_ERR, "error: file open/creation error!! errno:%s", strerror(errno));
	            return -1;
	        }
	
		//add while for receieving longer files
		while(flag == 0){
			recvret = recv(acceptfd, buf, MAXSIZE-1, 0);
			if(recvret == 0){
				syslog(LOG_ERR, "Connection_lost");
				return -1;
			}
			else if(recvret == -1){
				if(errno == EAGAIN){
					syslog(LOG_ERR, "Please reallocate space!!");
				}
				syslog(LOG_ERR, "errno: %s", strerror(errno));
			}
			else{
				strcpy(buf2, buf);				
				flag = 1;
			}
		}
		flag = 0;	

		writeret = write(storefd, buf, recvret);
		if(writeret == -1){
			syslog(LOG_ERR, "Write error!!");
		}


			ret = lseek(storefd, 0, SEEK_SET);
			if(ret == (off_t) -1){
				syslog(LOG_ERR, "lseek error!!");
			}


			readret = read(storefd, buf2, MAXSIZE);
			if(readret == -1){
				syslog(LOG_ERR,"read error!!");
			}	
			printf("readret:%d buf2:%s\n",readret, buf2);


			sendret = send(acceptfd, buf2, readret, 0);
			if(sendret == -1){
				syslog(LOG_ERR,"send error!!");
			}
	   	
//	     close(acceptfd);
//	     close(storefd);

	      if (signal(SIGINT, sig_handler) == SIG_ERR)
		  printf("\ncan't catch SIGINT\n");
	      else if (signal(SIGTERM, sig_handler) == SIG_ERR)
		  printf("\ncan't catch SIGINT\n");
	}

	closelog();

	return 0;
}
