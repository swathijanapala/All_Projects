#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#define max 1000


void arguments_fun(char input[],char *args[max],int *v){
	char* token = strtok(input, " \n");	
	while (token != NULL) {
		args[(*v)++]=token;
		token = strtok(NULL, " \n");	
	}
	
}


void free_args(char *args[]){
	for(int i=0;i<max;i++){
		args[i]=NULL;
	}

}


void main(){
	
	while(1){
		char input[max],*args[max];
		printf("shell>>");
		fgets(input,max,stdin);
				
		int no_of_arguments=0;
		arguments_fun(input,args,&no_of_arguments);
	
		char exited[]="exit";
    		if(strcmp(args[0],exited)==0){
    			exit(1);
    		}

		char help_command[]="help";

		if(strcmp(args[0],help_command)==0){
			printf("List of commands:\n");
            		printf("1. pwd - Show the present working directory\n");
            		printf("2. cd <directory_name> - Change the working directory\n");
            		printf("3. mkdir <directory_name> - Create a new directory\n");
            		printf("4. ls <flag> - List directory contents\n");
            		printf("5. exit - Exit the shell\n");
            		printf("6. help - Display this list of commands\n");
			continue;
    		}

		pid_t child_pid;
    		child_pid = fork();

    		if (child_pid == -1) {
        		perror("Fork failed");
        		exit(1);
    		}
    		if (child_pid == 0) {

			char cd_array[]="cd";
			if(strcmp(args[0],cd_array)==0){
				if (chdir(args[1]) == -1) {
            				perror("chdir");
            				exit(1);
        			}
        			char wd[1000];
        			printf("%s\n",getcwd(wd,sizeof(wd)));
				exit(1);
			}
       			else if(execvp(args[0],args)==-1){
       				perror("execlp failed");
            			exit(1);
            		}           
       		}      
    		else {
        		int status;
        		wait(&status);
        		if (WIFEXITED(status)) {
            			printf("Child exited with status: %d\n", WEXITSTATUS(status));
        		}
			free_args(args);
    		}
		
		
	}

}		



