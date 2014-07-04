# [1] MACRO DEFINITIONS
# [1.1] Commands
# C compiler
CC = arm-none-eabi-gcc
# C compiler flags
CFLAGS = -mcpu=arm7tdmi -c -o
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
	@echo "Converting ELF to UARM..."
	$(UC) -k p2test

# Linking
p2test: p2test.o pcb.o asl.o initial.o scheduler.o exceptions.o interrupts.o
	@echo "Linking..."
	$(LD) -T $(LDSCRIPTS) -o p2test $(CRTSO) $(LIBUARM) p2test.o pcb.o asl.o initial.o scheduler.o exceptions.o interrupts.o

# Compiling
p2test.o: $(P2TEST)
	@echo "Compiling p2test..."
	$(CC) $(CFLAGS) p2test.o $(P2TEST)

initial.o: $(INITIAL)
	$(CC) $(CFLAGS) initial.o $(INITIAL)

scheduler.o: $(SCHEDULER)
	$(CC) $(CFLAGS) scheduler.o $(SCHEDULER)

exceptions.o: $(EXCEPTIONS)
	$(CC) $(CFLAGS) exceptions.o $(EXCEPTIONS)

interrupts.o: $(INTERRUPTS)
	$(CC) $(CFLAGS) interrupts.o $(INTERRUPTS)

pcb.o: $(PCB)
	@echo "Compiling PCB..."
	$(CC) $(CFLAGS) pcb.o $(PCB)

asl.o: $(ASL)
	@echo "Compiling ASL..."
	$(CC) $(CFLAGS) asl.o $(ASL)

# [3] CLEANING
clean:
	rm -rf *o all