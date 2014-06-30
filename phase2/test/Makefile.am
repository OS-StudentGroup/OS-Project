# Makefile di phase2

# Dichiarazione delle cartelle base
INCLUDE = ../../include
PHASE1PATHE = ../../phase1/e
PHASE2PATHE = ../e
PHASE1PATHSRC = ../../phase1/src
ELFPATH = /usr/include/uMPS
ELF32 = /usr/share/uMPS

# Dichiarazione dei comandi base
CFLAGS = -Wall -I $(INCLUDE) -I $(PHASE2PATHE) -I $(PHASE1PATHE) -I $(PHASE1PATHSRC) -I $(ELFPATH) -I $(ELF32) -c
CC = mipsel-linux-gcc

# Target principale
all: initial.o scheduler.o exceptions.o interrupts.o p2test.0.1.o

initial.o: initial.c
	$(CC) $(CFLAGS) initial.c

scheduler.o: scheduler.c
	$(CC) $(CFLAGS) scheduler.c

exceptions.o: exceptions.c
	$(CC) $(CFLAGS) exceptions.c

interrupts.o: interrupts.c
	$(CC) $(CFLAGS) interrupts.c

p2test.0.1.o: p2test.0.1.c
	$(CC) $(CFLAGS) p2test.0.1.c

# Pulizia dei file oggetto
clean:
	rm -f *.o
