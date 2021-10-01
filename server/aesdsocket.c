#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#define MYPORT "9000"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold

int main(void){

	int status, socketfd, bindret, listenret, acceptfd, storefd, recvret, writeret, readret, sendret;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_storage connection_addr;
	socklen_t addr_size;
	char* store = "/var/tmp/aesdsocketdata";
	char* buf; char* buf2;
	struct stat buffer;

	openlog(NULL,0,LOG_USER);
	memset(&hints, 0, sizeof(hints)); 		//make sure the struct is empty
	hints.ai_family 	= AF_UNSPEC;		//IPV4/IPV6
	hints.ai_socktype 	= SOCK_STREAM;		//TCP stream sockets
	hints.ai_flags		= AI_PASSIVE;		//fill IP automatically
	
	//getaddrinfo
 	if ((status = getaddrinfo(NULL, MYPORT, &hints, &res)) !=0){
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	    syslog(LOG_ERR, "error: getaddrinfo!! errno: %s", strerror(errno));
   	    return -1;
	}
	
	//get socketfd
	socketfd = socket(PF_INET6, SOCK_STREAM, 0);
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

        storefd = open(store, O_CREAT | O_APPEND | O_RDWR, 755);
        if(storefd == -1){
            syslog(LOG_ERR, "error: file open/creation error!! errno:%s", strerror(errno));
            return -1;
        }

	
	while(1){
		//accept for a connection
		addr_size = sizeof connection_addr;
		acceptfd = accept(socketfd, (struct sockaddr*)&connection_addr, &addr_size);
		if(acceptfd == -1){
		    syslog(LOG_ERR, "error: accept!! errno:%s", strerror(errno));
		    return -1;
		}
	
		syslog(LOG_INFO,"Accepted connection from %d", acceptfd);

		//recieve data
		
			while((recvret = recv(acceptfd, buf, 2000, 0))>0){					//
				
				writeret = write(storefd, buf, sizeof(buf));
			}

                                if(recvret == 0){
                                        syslog(LOG_INFO, "Connection lost!!");
                                        break;
                                }
                                
                                else if(recvret == -1){
                                        syslog(LOG_INFO, "Recieve error!!");
                                        break;
                                }			

			stat(store, &buffer);
			readret = read(storefd, buf2, buffer.st_size);
			sendret = send(acceptfd, buf2, buffer.st_size, 0);
	    
	     close(socketfd);
	}

	closelog();

	return 0;
}
