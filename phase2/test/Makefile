# [1] MACRO DEFINITIONS
# [1.1] Commands
# C compiler
CC = arm-none-eabi-gcc
# C compiler flags
CFLAGS = -ansi -pedantic -Wall -c
# Linker
LD = arm-none-eabi-ld
# UARM converter
UC = elf2uarm

# [1.2] Addresses
# UARM library
LIBUARM = /usr/include/uarm/libuarm.o
# Linker scripts
LDSCRIPTS = /usr/include/uarm/ldscripts/elf32ltsarm.h.uarmcore.x
# C run-time start-off
CRTSO = /usr/include/uarm/crtso.o
# Source code
P2TEST = p2test.c
PCB = ../../phase1/c/pcb.c
ASL = ../../phase1/c/asl.c
INTERRUPTS = ../c/interrupts.c
EXCEPTIONS = ../c/exceptions.c
INITIAL = ../c/initial.c
SCHEDULER = ../c/scheduler.c

# [2] RULE DEFINITIONS
# Main target
all: uarm
	@echo "Done!"
# Converting ELF to UARM
uarm: p2test
	@echo "Converting..."
	$(UC) -k p2test

# Linking
p2test: p2test.o pcb.o asl.o initial.o scheduler.o exceptions.o interrupts.o
	@echo "Linking..."
	$(LD) -T $(LDSCRIPTS) $(CRTSO) p2test.o pcb.o asl.o initial.o scheduler.o exceptions.o interrupts.o $(LIBUARM) -o p2test

# Compiling
p2test.o: $(P2TEST)
	$(CC) $(CFLAGS) $(P2TEST)

initial.o: $(INITIAL)
	$(CC) $(CFLAGS) $(INITIAL)

scheduler.o: $(SCHEDULER)
	$(CC) $(CFLAGS) $(SCHEDULER)

exceptions.o: $(EXCEPTIONS)
	$(CC) $(CFLAGS) $(EXCEPTIONS)

interrupts.o: $(INTERRUPTS)
	$(CC) $(CFLAGS) $(INTERRUPTS)

pcb.o: $(PCB)
	$(CC) $(CFLAGS) $(PCB)

asl.o: $(ASL)
	@echo "Compiling..."
	$(CC) $(CFLAGS) $(ASL)

# [3] CLEANING
clean:
	rm -rf *o all