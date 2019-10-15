sshell	:	sshell.o
		gcc -o sshell sshell.o

sshell.o :	sshell.c
		gcc -c -Wall -Werror sshell.c

.PHONY	:	clean
clean 	:
		rm sshell.o sshell
