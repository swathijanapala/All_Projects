#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#define max 1000


void commands_fun(char *input,char *commands[],int *no_of_commands){
	char* token = strtok(input, "|\n");
	while (token != NULL) {
		commands[(*no_of_commands)++]=token;
		token = strtok(NULL, "|\n");
	}
}

void arguments_fun(char input[],char *args[max],int *v){
	char* token = strtok(input, " \n");	
	while (token != NULL) {
		args[(*v)++]=token;
		token = strtok(NULL, " \n");	
	}
	
}

void execute_pipe(char *args[],int prev_fd,int cur_fd[],int i,int no_of_commands){
	
	pid_t child_pid;
    	child_pid = fork();
	
    	if (child_pid == -1) {
        	perror("Fork failed");
        	exit(1);
    	}	
    	if (child_pid == 0) {
		if(i==no_of_commands-1){
			dup2(prev_fd,0);
			close(prev_fd);
		}
		else if(i==0){
			dup2(cur_fd[1],1);
			close(cur_fd[1]);
		}
		else{
			dup2(prev_fd,0);
			dup2(cur_fd[1],1);
			close(prev_fd);
			close(cur_fd[1]);
		}
       		if( execvp(args[0],args)<0){
       			perror("execvp failed");
            		exit(1);
            	} 
	}          
    	else {		
            		int status;
            		wait(&status);
            		if (WIFEXITED(status)) {
               			printf("Child exited with status: %d\n", WEXITSTATUS(status));
            		}
    		}	

}

void free_args(char *args[]){
	for(int i=0;i<max;i++){
		args[i]=NULL;
	}

}

void main(){
	
	while(1){
		char input[max],*commands_array[max];
		int no_of_commands=0;
		printf("shell>>");
		fgets(input,max,stdin);
		
		char exited[]="exit";
		commands_fun(input,commands_array,&no_of_commands);
    		if(strcmp(commands_array[0],exited)==0){
    			exit(1);
    		}
		
		
		int prev_fd;
		prev_fd=0;
		for(int i=0;i<no_of_commands;i++){
			int cur_fd[2];
			if (pipe(cur_fd) == -1) {
            			perror("Pipe failed");
           			exit(1);
        		}
			
			int arguments=0;	
			char* args[max];
			arguments_fun(commands_array[i],args,&arguments);
			execute_pipe(args,prev_fd,cur_fd,i,no_of_commands);
			prev_fd=cur_fd[0];
			close(cur_fd[1]);
			free_args(args);
		}
		free_args(commands_array);
	}

}		



