#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#define max 1000

void arguments_fun(char input[],char *args[max]){
	int v=0;
	char* token = strtok(input, " ");	
	while (token != NULL) {
		args[v++]=token;
		token = strtok(NULL, " ");	
	}
	
}
int append(char *output,char *buf,int *v){
	int i=0;
	while(buf[i]!='\0'){
		if(buf[i]=='\\'){
			i++;
			continue;
		}
		output[(*v)++]=buf[i];
		i++;
	}
	output[(*v)]='\0';
	int len = strlen(buf);
    	if(len > 0 && buf[len - 1] == '\\')
		return 0;
	else return 1;
	
}

void multiline_command(char *buf){
	char output[max];
	int v=0;
	if(append(output,buf,&v)){
			printf("%s\n",output);
			return;
	}
	while(1){
		char *buf;
		if (buf){
      			free(buf);
      			buf = (char *)NULL;
    		}
		buf = readline(">");
		if(append(output,buf,&v)){
			printf("%s\n",output);
			return;
		}
		
	}
}
void free_args(char *args[]){
	for(int i=0;i<max;i++){
		args[i]=NULL;
	}

}

void main()
{
	while(1){
		char *buf;
		if (buf){
      			free(buf);
      			buf = (char *)NULL;
    		}
		buf = readline("shell>>");
		char line[max];
		strcpy(line,buf);

		if(strcmp(buf,"exit")==0){
        		exit(1);
		}
		
		if(buf && *buf){
        		add_history(buf);
		}
		char* args[max];
		arguments_fun(buf,args);
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
		
		char echoing[]="echo";

		if(strcmp(echoing,args[0])==0){
			multiline_command(line+5);
			free_args(args);
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

       			else if( execvp(args[0],args)==-1){
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
			free_args(args);
    		}
	}
	
}







