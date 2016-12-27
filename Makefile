CC=gcc
CFLAGS=-c -g -Wall -std=c99

all: msh

msh: minishell.o arg_parse.o builtInFunc.o
	$(CC) -o msh minishell.o arg_parse.o builtInFunc.o
    
minishell.o: minishell.c $(HEADERS)
	$(CC) $(CFLAGS) minishell.c

arg_parse.o: arg_parse.c $(HEADERS)
	$(CC) $(CFLAGS) arg_parse.c
    
builtInFunc.o: builtInFunc.c $(HEADERS)
	$(CC) $(CFLAGS) builtInFunc.c
    
clean:
	rm -f *o msh
