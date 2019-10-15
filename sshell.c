#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct myobj {
	char **command; 		//contains full command with arguments
	char **argument;		//contains command and arguments separated
	char **pipeCmd; 		//contains commmand and argument for one pipe
};

void display_prompt() {
	//opening statement + repeating first statement
	printf("sshell$ ");
}

int getPipe(struct myobj *cmd) {
	int numPipe = 0;				//holds number of pipes in full command line
	int counter = 0;				//iterate through each word in command line to find '|'

	while(cmd->argument[counter] != NULL) {
		if(strcmp(cmd->argument[counter], "|") == 0) {
			numPipe++;
		}
		counter++;
	}
	return numPipe;
}

int addCmd(struct myobj *cmd) {
	char buf[512];							//buffer to hold user input
	char *temp = malloc(512);				//used to copy in original user input
	char *temp2 = malloc(512);				//used to copy in original user input
	cmd->command = malloc(512);				//will hold the full command line in command[0]
	cmd->argument = malloc(512);			//holds each word in command line 

	fgets(buf, sizeof(buf), stdin);			//reads input and stores into buf

	if (!isatty(STDIN_FILENO)) {
        printf("%s", buf);
        fflush(stdout);
  }

	strcpy(temp, buf);									//copying to non tamper with original command

	cmd->command[0] = strtok(temp, "\n");				//getting rid of the new line from full command list

	//code that splits the full command list into argument by token whitespace
	int counter = 0;
	cmd->argument[counter] = strtok(cmd->command[0], " ");
	while(cmd->argument[counter] != NULL) {
		cmd->argument[counter+1] = strtok(NULL, " ");
		counter++;
	}
	cmd->argument[counter+1] = NULL;

	//restoring original command line back into command[0]
	strcpy(temp2, buf);
	cmd->command[0] = strtok(temp2, "\n");

	//code that splits the full command list into command by token '|'; later used for pipes
	int counter2 = 1;
	cmd->command[1] = strtok(cmd->command[0], "|");
	while(cmd->command[counter2] != NULL) {
		cmd->command[counter2+1] = strtok(NULL, "|");
		counter2++;
	}
	//restoring original full command list
	cmd->command[0] = strtok(buf, "\n");

	//looks for commands not properly white spaced and adding white space
	int counter3 = 0;
	while(cmd->argument[counter3] != NULL) {

		const char ch = '<';
		const char ch2 = '>';
		const char ch3 = '|';
		char *ret = strchr(cmd->argument[counter3], ch);
		char *ret2 = strchr(cmd->argument[counter3], ch2);
		char *ret3 = strchr(cmd->argument[counter3], ch3);

		//for example parsing "grep hello<file" to "grep hello < file" 
		if(ret != NULL && strlen(cmd->argument[counter3]) != 1){
			cmd->argument[counter3] = strtok(cmd->argument[counter3], "<");
			cmd->argument[counter3+1] = "<";
			cmd->argument[counter3+2] = strtok(NULL, "<");
			cmd->argument[counter3+3] = NULL;
		}
		
		//for example parsing "echo dog>t" to "echo dog > t"
		if(ret2 != NULL && strlen(cmd->argument[counter3]) != 1){
			cmd->argument[counter3] = strtok(cmd->argument[counter3], ">");
			cmd->argument[counter3+1] = ">";
			cmd->argument[counter3+2] = strtok(NULL, ">");
			cmd->argument[counter3+3] = NULL;
		}

		//for example parsing "echo hello|grep hello" to "echo hello | grep hello"
		if(ret3 != NULL && strlen(cmd->argument[counter3]) != 1){
			cmd->argument[counter3] = strtok(cmd->argument[counter3], "|");
			cmd->argument[counter3+1] = "|";
			cmd->argument[counter3+2] = strtok(NULL, "|");
			cmd->argument[counter3+3] = NULL;
		}
		counter3++;
	}
	//free memory
	return 0;
}

//this function checks if command is either pwd, cd, or exit
//return value: 1 - pwd, 2 - cd, 3 - exit
int checkBuiltIn(struct myobj *cmd) {

	int first = strcmp(cmd->argument[0], "pwd");
	int second = strcmp(cmd->argument[0], "cd");
	int third = strcmp(cmd->argument[0], "exit");

	if (first == 0) {
		return 1;
	} else if (second == 0) {
		return 2;
	} else if (third == 0) {
		return 3;
	}
	return 0;
}

//function only runs if "<" or ">" is given in command
void reDirection(struct myobj *cmd) {
	int counter = 0;
	//loops until read through whole command line
	while (cmd->argument[counter] != NULL) {

		//checks if ">" is in command line
		if(strcmp(cmd->argument[counter], ">") == 0) {

			//opening to write into file at cmd->argument[counter+1]
			int out = open(cmd->argument[counter+1], O_RDWR|O_CREAT|O_TRUNC, 0644);
				
			dup2(out, STDOUT_FILENO);						//connecting stdout to file
			close(out);										//closing file
				
			cmd->argument[counter] = '\0';					//removing ">" and file name from argList
			break;											
		}
		
		//checks if "<" is in command line 
		if(strcmp(cmd->argument[counter], "<") == 0) {

			cmd->argument[counter] = cmd->argument[counter+1];		//removing "<" and replacing it with file name
			cmd->argument[counter+1] = NULL;						//file name moved so it is now null pointer

			int in = open(cmd->argument[counter], O_RDWR);			//opening of file
			dup2(in, STDIN_FILENO);									//connecting stdin to file
			close(in);												//closing file
		}
		counter++;
	}
}

//function only runs if "<" or ">" is given in command for each pipe command
void reDirectionPipe(struct myobj *cmd) {
	int counter = 0;
	//loops through one command side of a pipe to find "<" or ">"
	while (cmd->pipeCmd[counter] != NULL) {

		//finding ">"; writing to file 
		if(strcmp(cmd->pipeCmd[counter], ">") == 0) {

			int out = open(cmd->argument[counter+1], O_RDWR|O_CREAT|O_TRUNC, 0644);		//opening file to write
			dup2(out, STDOUT_FILENO);													//connecting stdout to file
			close(out);																	//closing file

			cmd->pipeCmd[counter] = '\0';												//removing ">" and filename from argList
			break;

		}

		//finding "<"; read from file
		if(strcmp(cmd->pipeCmd[counter], "<") == 0) {

			cmd->pipeCmd[counter] = cmd->pipeCmd[counter+1];				//replacing "<" with file name
			cmd->pipeCmd[counter+1] = NULL;									//now file name is null terminator

			int in = open(cmd->pipeCmd[counter], O_RDWR);					//opening file to read
			dup2(in, STDIN_FILENO);											//connecting stdin to file
			close(in);														//closing file
		}
		counter++;
	}
}

//finding errors from user command
int redirectionErr(struct myobj *cmd, int numPipe) {
	
	//checking if there is too many arguments (max 16) - 							
	int argumentLength = sizeof(cmd->argument)/sizeof(cmd->argument[0]);		
	if(argumentLength > 16) {				
		return 1;
	}
	
	//checking for redirection erros
	int counter = 0;
	while (cmd->argument[counter] != NULL) {

		//errors handling ">"
		if(strcmp(cmd->argument[counter], ">") == 0) {
			//if no file name 
			if(cmd->argument[counter+1] == NULL) {
				return 6;
			}
			int out = open(cmd->argument[counter+1], O_RDWR|O_CREAT|O_TRUNC, 0644);
			close(out);
			//if not able to access output file
			if(out == -1) {
				return 7;
			}	
		}

		//errors handling "<"
		if(strcmp(cmd->argument[counter], "<") == 0) {

			int in = open(cmd->argument[counter+1], O_RDWR);
			close(in);
			//if no file name
			if(cmd->argument[counter+1] == NULL) {
				return 4;
			}
			//if not able to access input file
			if(in == -1) {
				return 5;
			}
		}
		counter++;
	}
	
	//numPipe holds how many pipes
	if(numPipe > 0) {
		const char ch = '>';
		const char ch2 = '<';
		char *ret = strchr(cmd->command[1], ch);
		char *ret2 = strchr(cmd->command[numPipe-1], ch2);	
		//mislocated output redirection
		if(ret != NULL) {
			return 8;
		}
		//mislocated input redirection
		if(ret2 != NULL) {
			return 9;
		}
	}
	return 0;			//no error
}

void sigchld_handler(int signum) {
  int pid, status, serrno;
  serrno = errno;
  while (1)
    {
      pid = waitpid (WAIT_ANY, &status, WNOHANG);
      if (pid < 0)
        {
          break;
        }
      if (pid == 0){
        printf("ended\n");
		break;
	  }
    }
  errno = serrno;
}

//code that forks and runs the execvp command (does not run pipe commands)
int executeCmd(struct myobj *cmd, int bf) {
	pid_t pid;
	int status;
	pid = fork();
	
	if (pid != 0) {						//parent process
		if(bf == 0) {					//yes background command
			
			//keeps checking for any background and current process until finds terminated process or error occurs
			while(1) {
				pid = waitpid(WAIT_ANY, &status, WNOHANG);
				//there was error
				if (pid < 0) {
					break;
				}
				//something terminated
				if (pid > 0) {
					
					
					if(getpid() == pid) {
						fprintf(stderr, "+ completed '%s' [%d]\n", cmd->command[0], WEXITSTATUS(status));
					} else {
						fprintf(stderr, "+ completed '%s' [%d]\n", cmd->command[1], WEXITSTATUS(status));
				
					}
				}//end of termination
			}//end of while loop
			
		} else {
			//yes background
			cmd->command[1] = cmd->command[0];
		}
	} else {
		//child process
		//redirection validation
		reDirection(cmd);

		//runs command 
		execvp(cmd->argument[0], cmd->argument);
	
		if (errno == ENOENT) {
			fprintf(stderr, "Error: command not found\n");
		}
		exit(1);
	}
	return 0;
}

//code that runs only when pipes exist
int executePCmd(struct myobj *cmd, int x, int z) {
	int totalPipe = x*2;
	int mypipe[totalPipe];
	int status[x+1];
	pid_t pid;
	int num = 0;
	cmd->pipeCmd = malloc(512);
	
	for(int i = 0; i < x; i++) {
		pipe(mypipe + num);
		num *= 2;
	}

	int y = 1;
	//repeat for each pipe
	for(int m = 0; m < x+1; m++) {
		pid = fork();
		if (pid == 0) {
			//child - read
			if(m == x) {
			 	dup2(mypipe[y-1],0);
			 	for(int k = 0; k < totalPipe; k++) {
					close(mypipe[k]);
				}
				int l = 0;
				cmd->pipeCmd[0] = strtok(cmd->command[x+1], " ");
				while(cmd->pipeCmd[l] != NULL) {
					cmd->pipeCmd[l+1] = strtok(NULL, " ");
					l++;
				}
				cmd->pipeCmd[l] = NULL;

				reDirectionPipe(cmd);

				execvp(cmd->pipeCmd[0], cmd->pipeCmd);
				//perror("execvp");

				exit(1);


		 	} else if (m == 0) {
				dup2(mypipe[y],1);
				for(int k = 0; k < totalPipe; k++) {
					close(mypipe[k]);
				}
				int l = 0;
				cmd->pipeCmd[0] = strtok(cmd->command[1], " ");
				while(cmd->pipeCmd[l] != NULL) {
					cmd->pipeCmd[l+1] = strtok(NULL, " ");
					l++;
				}
				cmd->pipeCmd[l] = NULL;

				reDirectionPipe(cmd);

				execvp(cmd->pipeCmd[0], cmd->pipeCmd);
				//perror("execvp");

				exit(1);
			} else {
				dup2(mypipe[y-1],0);
				y += 2;
				dup2(mypipe[y],1);
				for(int k = 0; k < totalPipe; k++) {
					close(mypipe[k]);
				}
				int l = 0;
				cmd->pipeCmd[0] = strtok(cmd->command[m+1], " ");
				while(cmd->pipeCmd[l] != NULL) {
					cmd->pipeCmd[l+1] = strtok(NULL, " ");
					l++;
				}
				cmd->pipeCmd[l] = NULL;
				
				reDirectionPipe(cmd);

				execvp(cmd->pipeCmd[0], cmd->pipeCmd);
				//perror("execvp");

				exit(1);
			}
		}
	}//end of for loop
	for(int k = 0; k < totalPipe; k++) {
		close(mypipe[k]);
	}

	for(int j = 0; j < (x+1); j++) {
		
		waitpid(-1, &status[j], 0);
	}
	fprintf(stderr, "+ completed '%s' ", cmd->command[0]);
	
	for(int j = 0; j < x+1; j++) {
		fprintf(stderr, "[%d]", WEXITSTATUS(status[j]));
	}
	fprintf(stderr, "\n");
	return 0;
}//end of execute command function

//check if "&" exist in command 
int checkBackground(struct myobj *cmd) {
	int background = 0;					//return value 0 - no background, 1 - yes background
	int counter = 0;
	const char ch = '&';
	
	//loops through whole command list
	while(cmd->argument[counter] != NULL) {
		char *ret = strchr(cmd->argument[counter], ch);					//checks each word in command line to "&"
		
		//ret is NULL if word does not contain "&"
		if(ret != NULL) {						
			cmd->argument[counter] = strtok(cmd->argument[counter], "&");		//removes "&" 
			cmd->argument[counter+1] = NULL;									//replace "&" with null terminator
			background = 1;														//yes background exist
			break;
		}
		counter++;
	}
	return background;
}

//code for current working directory 
char *gnu_getcwd () {
	size_t size = 100;

	while(1) {
		char *buffer = (char *) malloc (size);
		if(getcwd (buffer, size) == buffer)
			return buffer;
		free (buffer);
	  //if (errno != ERANGE)
	  //  return 0;
		size *= 2;
	}
}

//execute pwd
void doPwd(struct myobj *cmd) {

	char *pwd = gnu_getcwd();			//get current working directory

	//if no error 0 else if error 1
	if (pwd != NULL) {
		printf("%s\n", pwd);
		fprintf(stderr, "+ completed '%s' [0]\n", cmd->command[0]);
	} else {
		fprintf(stderr, "+ completed '%s' [1]\n", cmd->command[0]);
	}
}

//execute cd
void doCd(struct myobj *cmd) {

	chdir(cmd->argument[1]);				//argument[1] contains path name
	//if no such directory/file name; error - 1, no error = 0
	if(errno == ENOENT) {
		fprintf(stderr, "Error: no such directory\n");
		fprintf(stderr, "+ completed '%s' [%d]\n", cmd->command[0], 1);
	} else {
		fprintf(stderr, "+ completed '%s' [%d]\n", cmd->command[0], 0);
	}
}

//handle errors return from redirectionErr
void errorHandle(int error) {

	if(error == 1) {
		fprintf(stderr, "Error: too many process arguments\n");
	} else if (error == 2) {
		fprintf(stderr, "Error: missing command\n");
	} else if (error == 3) {
		fprintf(stderr, "Error: command not found\n");
	} else if (error == 4) {
		fprintf(stderr, "Error: no input file\n");
	} else if (error == 5) {
		fprintf(stderr, "Error: cannot open input file\n");
	} else if (error == 6) {
		fprintf(stderr, "Error: no output file\n");
	} else if (error == 7) {
		fprintf(stderr, "Error: cannot open output file\n");
	} else if (error == 8) {
		fprintf(stderr, "Error: mislocated output redirection\n");
	} else if (error == 9) {
		fprintf(stderr, "Error: mislocated input redirection\n");
	}
}

int main(int argc, char *argv[]) {
	while (1) {
		struct myobj s;						//struct that contains the command and arguments from user
		display_prompt();					//displays the staring prompt
		addCmd(&s);							//gets command and arguments from user/stdin
		int bInt = checkBuiltIn(&s);		// return value 0 = regular command, 1 = pwd, 2 = cd, 3 = exit
		
		
		int numPipe = getPipe(&s);				//return number of pipes
		int errors = redirectionErr(&s, numPipe);		//gets error, 0 for no error
		int bf = checkBackground(&s);		//check if background process
		
		//first checks if any error in command line
		//second if pipe exist
		//runs regular command 
		//then pwd
		//then cd
		//then exit
		if(errors != 0) {
			errorHandle(errors);
		} else if (numPipe != 0) {
			executePCmd(&s, numPipe, bf);
		} else if(bInt == 0) {
			//do regular command
			executeCmd(&s, bf);
		} else if (bInt == 1) {
			//do pwd
			doPwd(&s);
		} else if (bInt == 2) {
			//do cd
			doCd(&s);
		} else {
			//do exit
			fprintf(stderr, "Bye...\n");
			exit(0);
		}
	}
	return 0;
}
