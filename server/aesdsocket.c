/////////////////////////////////////////////////////////////////////////////////////////////////
//	File:		aesdsocket.c							       								   //
//	Author:		Michelle Christian						       								   //
//	ECEN 5713 AESD Assignment 5 part 1						       							   //
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
#include <pthread.h>
#include <time.h>
#include "queue.h"

/*******************************************DEFINES*********************************************/
#define MAXSIZE 100
#define MYPORT "9000"  			// the port users will be connecting to
#define BACKLOG 10     			// how many pending connections queue will hold

/***************************************GLOBAL VARIABLE*****************************************/

int storefd, acceptfd, socketfd, signalflag=0;	//aesdsocketdata file fd, client fd, server socket fd
pthread_mutex_t mutex;							//mutex to be passed for thread implementation
int mutex_ret=0;
struct sockaddr_in connection_addr;

/*************************************LINKED LIST***********************************************/
//structure to store thread params
struct params{
	bool thread_complete_flag;
	int  per_thread_acceptfd;
	pthread_t thread;
	int threadid;
};

//structure to store timer thread params
struct timerthread{
	int storefilefd;
};

//slist structure that stores thread params and pointer to next thread
typedef struct slist_data_s slist_data_t;
struct slist_data_s{
	struct params value;
	SLIST_ENTRY(slist_data_s) entries;
};

/**
* set @param result with @param ts_1 + @param ts_2
*/
static inline void timespec_add( struct timespec *result,
                        const struct timespec *ts_1, const struct timespec *ts_2)
{
    result->tv_sec = ts_1->tv_sec + ts_2->tv_sec;
    result->tv_nsec = ts_1->tv_nsec + ts_2->tv_nsec;
    if( result->tv_nsec > 1000000000L ) {
        result->tv_nsec -= 1000000000L;
        result->tv_sec ++;
    }
}


/**
* Setup the timer at @param timerid (previously created with timer_create) to fire
* every @param timer_period_ms milliseconds, using @param clock_id as the clock reference.
* The time now is saved in @param start_time
* @return true if the timer could be setup successfuly, false otherwise
*/
static bool setup_timer( int clock_id,
                         timer_t timerid, unsigned int timer_period_s,
                         struct timespec *start_time)
{
    bool success = false;
    if ( clock_gettime(clock_id,start_time) != 0 ) {
        printf("Error %d (%s) getting clock %d time\n",errno,strerror(errno),clock_id);
    } else {
        struct itimerspec itimerspec;
        itimerspec.it_interval.tv_sec = timer_period_s;
        itimerspec.it_interval.tv_nsec = 1000000;
        timespec_add(&itimerspec.it_value,start_time,&itimerspec.it_interval);
        if( timer_settime(timerid, TIMER_ABSTIME, &itimerspec, NULL ) != 0 ) {
            printf("Error %d (%s) setting timer\n",errno,strerror(errno));
        } else {
            success = true;
        }
    }
    return success;
}

//timer thread to print timestamps
void timer_thread (union sigval sigval)
{

	struct timerthread* timtd = (struct timerthread*) sigval.sival_ptr;
	time_t rawtime;
	struct tm *info;
	char strtime[80];
	size_t strbyte;

	time(&rawtime);

	info = localtime(&rawtime);

	strbyte = strftime(strtime, 80, "timestamp: %Y %b %a %H %M %S%n", info);

		if(strbyte == 0){
			perror("strftime failure!!!");
		}

	if(pthread_mutex_lock(&mutex)!=0){
		perror("mutex lock error");
		printf("mutex lock error in time thread");
	}

	int writeret = write(timtd->storefilefd, strtime, strbyte);
	if(writeret != strbyte){
		syslog(LOG_ERR, "Timestamp error!!!");
	}

	if(pthread_mutex_unlock(&mutex)!=0){
		perror("mutex unlock error");
		printf("mutex unlock error in time thread");
	}	
}

//thread functionality that receives, writes, reads and sends one packet 
void* thread_function(void* thread_param){

	struct params* param_value = (struct params*) thread_param;
	int recvret=0, writeret=0, readret=0, sendret=0;
	char buf[MAXSIZE];														    //static buffer to recieve the data into
	char* buf3; char* newline; char* newline2;									//buffer to read data from file and buffers to check newline characters
	size_t buf2_size=0;
	int sendsize=0;																//size of each line to be sent
	off_t ret;

	char *IP = inet_ntoa(connection_addr.sin_addr);
	syslog(LOG_DEBUG, "Accepted connection from %s\n", IP);

			//clear the buffer for input
	        memset(buf, '\0', sizeof(buf));

		//buffer to read all the values in and realloc if required
		char* buf2 = (char*)malloc(MAXSIZE*sizeof(char));
		if(buf2 == NULL){
			syslog(LOG_ERR, "read buffer malloc failed!!");
			goto end;
		}

		int mallocsize = MAXSIZE, reallocsize = MAXSIZE;		//realloc sizes for read and write buffer
		char* newptr = NULL; char* newptr2 =NULL;				//newptr checks the realloc return 
		int setback=0, pos=0, end_of_file=0, nextsize=0;
		
		do{
				//receive the bytes
				recvret = recv(param_value->per_thread_acceptfd, buf, sizeof(buf), 0);
				if(recvret == -1){
					syslog(LOG_ERR, "errno: %s", strerror(errno));
					pthread_exit(param_value);
				}
				else{
					if((mallocsize-buf2_size) < recvret){
						mallocsize += recvret;
						newptr = (char*)realloc(buf2, mallocsize* sizeof(char)); //realloc if the recived bytes are more than the size of buffer
						if(newptr!=NULL){
							buf2 = newptr;
						}
						else{
							goto end;
						}
					}
					memcpy(&buf2[buf2_size], buf, recvret);				//copy contents to another buffer so later can append bigger data files
				}

			buf2_size+=recvret;
			newline = strchr(buf, '\n');	//check for newline character
		}while(newline == NULL); 

		pthread_mutex_lock(&mutex);										//lock the write functionality
		//write into the file
        writeret = write(storefd, buf2, buf2_size);
            if(writeret == -1){
                syslog(LOG_ERR, "Write error!!");
				pthread_exit(param_value);
            }
		pthread_mutex_unlock(&mutex);	


		end_of_file = lseek(storefd, 0, SEEK_END);
            ret = lseek(storefd, 0, SEEK_SET);
                if(ret == (off_t) -1){
                    syslog(LOG_ERR, "lseek error!!");
					pthread_exit(param_value);
                }

		while(setback!= end_of_file){
			sendsize=0;
			
			buf3 = (char*)malloc(MAXSIZE*sizeof(char));
			if(buf3 == NULL){
				syslog(LOG_ERR,"writebuffer malloc failed!!");
				break;
			}

	                //clear the buffer for input
        	        memset(buf3, '\0', MAXSIZE);

			//nextsize calculates the size of the next line so that we just have to realloc that much
			nextsize = end_of_file - pos;

			do{
				pthread_mutex_lock(&mutex);										//lock the read functionality
		               readret = read(storefd, buf3+sendsize, sizeof(char));
	                        if(readret == -1){
	                                syslog(LOG_ERR,"read error!!");
									pthread_exit(param_value);
	                        }       
                pthread_mutex_unlock(&mutex);        							

				if(reallocsize - sendsize < end_of_file)
				{
					reallocsize += nextsize ;
					newptr2 = (char*)realloc(buf3, reallocsize*sizeof(char));
					if(newptr2 != NULL){
						buf3 =newptr2;
					}
					else{
						goto end;
					}
				}

				sendsize += readret;

				if(sendsize>1){
				newline2 = strchr(buf3, '\n');    //check for newline character
				}
			}while(newline2==NULL);

			pos = lseek(storefd, 0, SEEK_CUR);
			setback+=sendsize;
			
			//send data
           		sendret = send(param_value->per_thread_acceptfd, buf3, sendsize, 0);
                        if(sendret == -1){
                                syslog(LOG_ERR,"send error!!");
								pthread_exit(param_value);
                        }
	
     
end:			free(buf3);
	}
	   //free the malloced buffers to avoid memory leak	
	   free(buf2);
 	   close(acceptfd);	
   	   syslog(LOG_INFO,"Closed connection from %s",IP);	   

		param_value->thread_complete_flag = true;
		pthread_exit(param_value);
}

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

	int status, bindret, listenret;										 		//return values for respective functions
	struct addrinfo hints;														//hints addrinfo to get address		
	struct addrinfo *res;														//stores the result
	socklen_t addr_size;														//stores the address size of the connection			
	int opt=1;				
    bool daemon = false;														//boolean to check if daemon mode activated or not
	pid_t pid; 																	
	int id=0, rc=0;
	struct timerthread td;														//timerthread struct to store file descriptor
    struct sigevent sev;														//signal struct to pass timer thread
    timer_t timerid;															//timer to be used
	int clock_id = CLOCK_MONOTONIC;												//CLOCK used
	struct timespec start_time;

	//initialize the lined list
	slist_data_t *llpointer=NULL;
	SLIST_HEAD(slisthead, slist_data_s) head;
	SLIST_INIT(&head);

	//initialize the mutex
	pthread_mutex_init(&mutex, NULL);

	//open the log
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

	memset(&sev,0,sizeof(struct sigevent));
	        /**
        * Setup a call to timer_thread passing in the td structure as the sigev_value
        * argument
        */
		td.storefilefd = storefd;
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_value.sival_ptr = &td;
        sev.sigev_notify_function = timer_thread;
        if ( timer_create(clock_id,&sev,&timerid) != 0 ) {
            printf("Error %d (%s) creating timer!\n",errno,strerror(errno));
        }

		if(!(setup_timer(clock_id, timerid, 10, &start_time))){
			printf("Timer setup error!!");
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
                
				//setup the values in linked list for each entry
				llpointer = malloc(sizeof(slist_data_t));
				(llpointer->value).per_thread_acceptfd = acceptfd;
				(llpointer->value).threadid = id;
				llpointer->value.thread_complete_flag = false;

				//insert head for each entry
        		SLIST_INSERT_HEAD(&head, llpointer, entries);
				id++;

			//create thread
			rc = pthread_create(&((llpointer->value).thread), NULL, thread_function,(void*)&(llpointer->value));
			if(rc!=0){
				perror("Pthread create error!!");
				return -1;
			}
				
			//check if thread complete flag set if so join the thread
			SLIST_FOREACH(llpointer, &head, entries) {
    	    	if((llpointer->value).thread_complete_flag == true){
					pthread_join((llpointer->value).thread, NULL);
				}
    		}
	}
	
	//if signal encountered join all the threads unconditionally
	SLIST_FOREACH(llpointer, &head, entries) {
		pthread_join((llpointer->value).thread, NULL);
    }

	//empty the linked list and free the data
	while (!SLIST_EMPTY(&head)) {
        llpointer = SLIST_FIRST(&head);
        SLIST_REMOVE_HEAD(&head, entries);
        free(llpointer);
    }

	//close the file and log
	close(storefd);
	close(socketfd);
	close(acceptfd);
	closelog();
	
	//remove the file
	    if(remove("/var/tmp/aesdsocketdata") == -1)                 //delete the file
    	    {   
        	syslog(LOG_ERR,"file delete error!!");
    	    }    

	return 0;
}
