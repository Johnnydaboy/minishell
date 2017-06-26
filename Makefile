CC=gcc
CFLAGS=-c -g -Wall

all: msh

msh: minishell.o arg_parse.o builtInFunc.o expand.o
	$(CC) -o msh minishell.o arg_parse.o builtInFunc.o expand.o
    
minishell.o: minishell.c
	$(CC) $(CFLAGS) minishell.c

arg_parse.o: arg_parse.c
	$(CC) $(CFLAGS) arg_parse.c
    
builtInFunc.o: builtInFunc.c
	$(CC) $(CFLAGS) builtInFunc.c
    
expand.o: expand.c
	$(CC) $(CFLAGS) expand.c
    
clean:
	rm -f *o msh
