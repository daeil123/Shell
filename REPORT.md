## ECS 150 REPORT for HW1

## Phase 0-4

*  Made the  function ‘display_prompt()’ to display the starting prompt “sshell$” using ‘printf(“sshell$ “)'
* Used the struct named ‘myobj’ to get the command and arguments from user easier
* Made the function ‘addCmd()’ to get command and arguments from the user
* Made the function ‘executeCmd()’ to fork and execute command
* Made the function ‘doPwd’ and ‘doCd’ for excute 'pwd' and 'cd'
* Made the function ‘checkBuiltIn()’ to check if command is either pwd, cd, or exit

In the function addCmd(),
1. read user input and store it into buffer using ‘fgets()’ and ’strcpy()'
2. get rid of the new line from full command list using ’stroke( full command ,”\n”)'
3. split the full command list into argument by token whitespace using ’streak(full command , “ ")' 

In the function executeCmd(),
1. Wait until child process using ‘waitpid (WAIT_ANY, $status, WNOHANG)’
	WAIT_ANY : specifies that waitpid should return status information about any child process.
	WNOHANG : return immediately if no child has exited.
2. execute command by running the ‘execvp()’

In the function ‘doPwd()’,
1. Used the command ‘gnu_getcwd()’ to get current working directory. 
2. Set 0 for no error and 1 for error

In the function ‘doCd()’,
1. Used the command ‘chdir()’ to change  the current working directory to the directory specified in path. 
2. Set 0 for no error and 1 for error

In the function 'checkBuiltIn()'
1. Set return value: 1 for pwd, 2 for cd, 3 for exit 
2. If checkBuiltin() return 1, run ‘doPwd’. 
3. If checkBuiltin() retune 2, run ‘doCd’. 
4. Else run exit(0)

## Phase 5-6 : Input redirection, Output redirection
* Made the function ‘reDirection(struct myobj *cmd)’
* Made the function ‘redirectionErr()’ to find errors from user command

For Input redirection, 
1. Make loops until read through a whole command line
2. Check if “<“ is in the command line
3. If “<“ is founded, remove “<“ and replace it with the followed file name
4. Make the origin argument of the file name NULL
5. Open file and connect stdin to the file using the command ‘dup2’
6. Close file and repeat above process until the loops are finished. 

For output redirection,
1. Make loops until read through a whole command line
2. Check if “>” is in command line
3. If “>” is founded, open the followed file after “>"
4. Connect stdout to the file using the command ‘dup2’ and remove “>” and the file name form argument list
5. Close file and repeat above process until the loops are finished.

For redirection error, 
1. Check if there is too many arguments 
2. Check if there is no file name  
3. Check if not able to access output file 

## Phase 7 : Pipeline commands
* Struct ‘myobj’ contains char ‘pipecmd’ which is command and argument for one pipe
* Get the number of pipe ” l " using the function ‘getPipe()’
* Made the function ‘executePCmd()’ to execute the command when pipes exist

In the function 'executePCmd()',
When pid is 0 (child), 
1. Make loops until read through whole pipe number
2. If the pipe is the first one, duplicate stdout into write part of pipe
3. If the pipe is not the first and the last,  duplicate stdin into read part of the pipe and then duplicate stdout into write part of next pipe
4. If the pipe is the last one, duplicate stdin into read part of pipe

When pid is not 0 (parents),
1. wait until child process

## Phase 8 : Background commands

* Made the function checkBackground() to return value 1 if there is background (“&") value 0 if there is not (“&”)

If there is background,
1. Run the function ‘excuteCmd’. But, here, parent does not wait.
2. Copy the whole command from user into the char ‘pastCommand'

If there is no background,
1. Run the function ‘excuteCmd’ 
2. Execute same way as the original. Here, parent does wait with ‘WNOHANG’
3. Keep checking for any background and current process until finds terminated process or error occurs
4. If something is terminated, print the output or if some error happen, print the error








