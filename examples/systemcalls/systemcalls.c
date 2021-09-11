/** Edited by: Michelle Christian  **/
#define _XOPEN_SOURCE 											/* if we want WEXITSTATUS, etc. */

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "systemcalls.h"
/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
 *
 *
*/
    
   bool result = false;							//boolean to return if the system() function passes/ fails
   int status;								//status gives the return value of the function
   status=system(cmd);					
   	if(status == 0){						//Only if the return value is 0 the system() call has completed with success hence return true
		result = true;
	}
    return result;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/
    int status;								//status is passed into the waitpid function to get the staus of wait function
    pid_t pid;								//stores the return value of fork function

    //fork
    pid = fork();


    if(pid == -1)							//if the fork function returns -1 meaning the child process could not be created
	    return -1;

    else if (pid ==0){							//if the fork function returms 0 meaning the child process has successfully been created

	 //execv
	execv(command[0],command);					//call execv, pass first argument as command[0] and rest of the arguments as second parameter
									//command is passed because it will take the first paramter as filename and take rest of the arguments as input
	exit(-1);							//if execv fails exit from child process with status -1
    }

    //wait
    if(waitpid(pid, &status, 0) == -1){					//wait for the process to see if it executes successfully
	    return -1;							//if not return with status -1
    }

    else if (WIFEXITED(status)){					//check the status using WIFEXITED
	    if(WEXITSTATUS(status) == 0){				//if the status is zero meaning the process has been executed successfully
	    	return true;
	    }
	    else{							//else return false
	    	return false;
	    }
    }

    va_end(args);

    return true;

}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/
    int pid;								//pid to store the return value of fork
    int status;								//check the status for waitpid
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);		//open the outputfile for writinf the path in 
    if (fd < 0){perror("open");exit(-1);}				//return error if could not open the file and exit with status -1

    //fork
    pid = fork();							
    if(pid == -1){perror("fork"); exit(-1);}				//return error if child process not created and exit with status -1

    else if(pid ==  0){							
		if(dup2(fd,1)<0) {perror("dup2"); exit(-1);}		//redirect the file path to outputfile
		close(fd);						//close the file

	         //execv
        	execv(command[0],command);				//call execv, pass first argument as command[0] and rest of the arguments as second parameter
									//command is passed because it will take the first paramter as filename and take rest of the arguments as input

        	exit(-1);
     }
     else{close(fd);}							//close the file if none of the conditions satisfied

   //wait
    if(waitpid(pid, &status, 0) == -1){					//wait for the process to see if it executes successfully
            return -1; 							//if not return with status -
    }   
    else if (WIFEXITED(status)){					//check the status using WIFEXITED
            if(WEXITSTATUS(status) == 0){ 				//if the status is zero meaning the process has been executed successfully
                return true;
            }
            else{
                return false;						//else return false
            }
    }   

    va_end(args);
    
    return true;
}
