# MACRO DEFINITIONS
OBJS = 	../h/const.h 
		../h/types.h
		Makefile
CFLAGS = -ansi -pedantic -Wall -c
CC = gcc 

# EXPLICIT RULES
all: p1test.o asl.o pcb.o $(OBJS)
	$(CC) $(CFLAGS) main.c
	
p1test.o: p1test.c $(OBJS)
	$(CC) $(CFLAGS) p1test.c
 
asl.o: asl.h $(OBJS)
	$(CC) $(CFLAGS) asl.c

pcb.o: pcb.h $(OBJS)
	$(CC) $(CFLAGS) pcb.c

# CLEANING
clean:
	rm -f *.o