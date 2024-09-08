#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <pthread.h>
#define max 1000
#define MAX_LINES 1000
#define MAX_COLS 1000

int times[200][2];

struct VectorArgs {
    int *vector1;
    int *vector2;
    int start;
    int end;
};
int dot_result=0;

void *addvec_thread(void *arg) {
    struct VectorArgs *args = (struct VectorArgs *)arg;
    for (int i=args->start; i<=args->end; i++) {
        args->vector1[i] += args->vector2[i];
    }
    return NULL;
}

void *subvec_thread(void *arg) {
    struct VectorArgs *args = (struct VectorArgs *)arg;
    for (int i=args->start; i<=args->end; i++) {
        args->vector1[i] -= args->vector2[i];
    }
    return NULL;
}

void *dotprod_thread(void *arg) {
    struct VectorArgs *args = (struct VectorArgs *)arg;
    for (int i=args->start; i<=args->end; i++) {
        dot_result+=args->vector1[i] * args->vector2[i];
    }
    return NULL;
}

void threading(char *argv[],int argc){
	if (argc<4) {
        	fprintf(stderr, "Usage: %s <command> <file1> <file2> -<num_threads>\n", argv[0]);
        	exit(1);
    	}

    	char *command = argv[0];
    	char *file1 = argv[1];
    	char *file2 = argv[2];
    	int num_threads = 3;

    	if (argc==4) {
        	if (strncmp(argv[3], "-", 1) == 0) {
            		num_threads = atoi(argv[3] + 1);
        	}
    	}
	
    	int vector1[max];
    	int vector2[max];
    	int vector_size = 0;

    	FILE *file = fopen(file1, "r");
    	if (file == NULL) {
        	perror("Error opening file 1");
        	exit(1);
    	}

    	while (fscanf(file, "%d", &vector1[vector_size]) == 1) {
        	vector_size++;
    	}
    	fclose(file);

    	file = fopen(file2, "r");
    	if (file == NULL) {
        	perror("Error opening file 2");
        	exit(1);
    	}

    	int i = 0;
    	while(fscanf(file, "%d", &vector2[i]) == 1) {
        	i++;
    	}
    	fclose(file); 
    	if (vector_size <= 0) {
        	fprintf(stderr, "Invalid vector size or empty files.\n");
        	exit(1);
    	}
    
    	int count=num_threads;
    	int rem=vector_size%count, val=vector_size/count,  start=0;
	int flag = val > 0;
	for (int j=0;j<count;j++){
		if (flag) {
			times[j][0] = start;
			times[j][1] = start+val-1;
			start += val;
		}
		if (rem) {
			rem--;
			times[j][1]++;
			start++;
		}
	}

    	pthread_t threads[num_threads];
    	struct VectorArgs args[num_threads];

    	if (strcmp(command, "addvec") == 0) {
        	for (int i = 0; i < num_threads; i++) {
	    		args[i].vector1=vector1;
			args[i].vector2=vector2;
	   		args[i].start=times[i][0];
	   		args[i].end=times[i][1];
            		pthread_create(&threads[i], NULL, addvec_thread, &args[i]);
        	}

        	for (int i = 0; i < num_threads; i++) {
            		pthread_join(threads[i], NULL);
        	}
		printf("Result Vector: ");
    		for (int i = 0; i < vector_size; i++) {
        		printf("%d ", vector1[i]);
    		} 
    	}
    	else if (strcmp(command, "subvec") == 0) {
        	for (int i = 0; i < num_threads; i++) {
			args[i].vector1=vector1;
	   		args[i].vector2=vector2;
	   		args[i].start=times[i][0];
	   		args[i].end=times[i][1];
        	    	pthread_create(&threads[i], NULL, subvec_thread, &args[i]);
        	}

        	for (int i = 0; i < num_threads; i++) {
            		pthread_join(threads[i], NULL);
        	}
		printf("Result Vector: ");
    		for (int i = 0; i < vector_size; i++) {
        		printf("%d ", vector1[i]);
    		} 
    	}
    	else if (strcmp(command, "dotprod") == 0) {
		for (int i = 0; i < num_threads; i++) {
			args[i].vector1=vector1;
	   		args[i].vector2=vector2;
	   		args[i].start=times[i][0];
	   		args[i].end=times[i][1];
        	    	pthread_create(&threads[i], NULL, dotprod_thread, &args[i]);
        	}

        	for (int i = 0; i < num_threads; i++) {
            		pthread_join(threads[i], NULL);
        	}
		printf("Dot Product: %d",dot_result);
    	}
    	else {
        	fprintf(stderr, "Invalid command: %s\n", command);
        	exit(1);
    	}  
	printf("\n");
}
void arguments_fun(char input[],char *args[max],int *v){
	
	char* token = strtok(input, " ");	
	while (token != NULL) {
		args[(*v)++]=token;
		token = strtok(NULL, " ");	
	}
	
}

void commands_fun(char *input,char *commands[],int *no_of_commands){
	char* token = strtok(input, "|\n");
	while (token != NULL) {
		commands[(*no_of_commands)++]=token;
		token = strtok(NULL, "|\n");
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
void vi_command(char *args[]){

	
	FILE *file = fopen(args[1], "r+");
    	if (file == NULL) {
        	printf("Error: Cannot open file %s\n", args[1]);
        	return;
    	}
	
	initscr();
	noecho();
	keypad(stdscr, TRUE);

	int ch,y=0,x=0;
	char buffer[MAX_LINES][MAX_COLS] = {0};
    	int buffer_y = 0, buffer_x = 0;
	clear();
        

        fseek(file, 0, SEEK_SET);
        y=0;
        while((ch=fgetc(file))!= EOF) {
        	if(ch=='\n'){
            		move(++y, 0);
            		x = 0;
		}
		else if (ch>= 32 && ch <= 126) {
            		addch(ch);
            		x++;
            		buffer[buffer_y][buffer_x++] = ch;
        	}
    	}
    	refresh();
	fseek(file, 0, SEEK_END);

	int lines_modified = 0;
    	int words_modified = 0;
    	int characters_modified = 0;
	
	ch=getch();
	while (1) {

        ch = getch();

        if (ch == 27) {  				// Escape key to close
            break;
        } 
	else if (ch == 24) {  				// Ctrl + X to exit
            break;
	}
	else if (ch == KEY_LEFT && x > 0) {
            move(y, --x);
        } 
	else if (ch == KEY_RIGHT) {
            if (x < COLS - 1) {
                move(y, ++x);
            }
        } 
	else if (ch == KEY_UP && y > 0) {
            move(--y, x);
        } 
	else if (ch == KEY_DOWN) {
            if (y < LINES - 1) {
                move(++y, x);
            }
        } 
	else if (ch == KEY_BACKSPACE || ch == 127) {  	// Backspace key...
    		if (x > 0) {
        		int cur_x, cur_y;
        		getyx(stdscr, cur_y, cur_x);
        		move(y, x - 1);
        		delch();
        		x--;
			characters_modified++;
    		}
        } 

	else if (ch == 10) {  				// Enter key...
		if (y < LINES - 1) {
	                if (x < COLS - 1) {
				move(y, x);
				insch('\n');
				y++;
	                        for(int i = buffer_y; i > y; i--) {
	                        	for(int j = buffer_x; j < MAX_COLS; j++) {
	                            		buffer[i][j] = buffer[i - 1][j];
	                        	}
	                    	}
	                    	buffer[buffer_y + 1][0] = '\0';
	                    	buffer_x = 0;
                		} 
			else {
                    		x = 0;
                    		y++;
                    		buffer_y++;
                    		buffer_x = 0;
                	}
			lines_modified++;
            	}
        }

	else if(ch >= 32 && ch <= 126 && x < COLS - 1) {  // insert characters..
            move(y, x);
            insch(ch);
            x++;
            buffer[buffer_y][buffer_x++] = ch;
	    if(ch==' '){
		words_modified++;
	    }
	    else{
	    characters_modified++;}
        } 
	else if (ch == 19) { 				 // Ctrl+s to save..
            fseek(file, 0, SEEK_SET);
            for (int i = 0; i <= buffer_y; i++) {
                for (int j = 0; j < buffer_x; j++) {
                    fputc(buffer[i][j], file);
                }
            }
            fflush(file);
        }
	refresh();
    }
	fclose(file);
	endwin();
	printf("Lines modified: %d\nWords modified: %d\nCharacters modified: %d\n", lines_modified, words_modified, characters_modified);
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

void piping(char *input){
		char *commands_array[max];
		int no_of_commands=0;
		commands_fun(input,commands_array,&no_of_commands);
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

int pipe_there_or_not(char args[]){
	
	int i=0;
	while(args[i]!='\0'){
		if(args[i]=='|'){
			return 1;
		}
		i++;
	}
	return 0;
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
		char help_command[]="help";

		int no_of_arguments=0;
		char* args[max];
		arguments_fun(buf,args,&no_of_arguments);

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
		char vi_array[]="vi";
		char pipe_array[]="pipe";
		char *vec[3];
		vec[0]="addvec";
		vec[1]="subvec";
		vec[2]="dotprod";

		if(strcmp(echoing,args[0])==0){
			multiline_command(line+5);
			free_args(args);
			continue;
		}	
		else if(strcmp(vi_array,args[0])==0){
			vi_command(args);
			free_args(args);
			continue;
		}
		else if(strcmp(vec[0],args[0])==0 || strcmp(vec[1],args[0])==0 || strcmp(vec[2],args[0])==0){
			threading(args,no_of_arguments);
			free_args(args);
			continue;
		}
		
		if(pipe_there_or_not(line)){
			piping(line);
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







