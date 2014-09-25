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
P1TEST = p1test.c
PCB = ../c/pcb.c
ASL = ../c/asl.c

# [2] RULE DEFINITIONS
# Main target
all: uarm
	@echo "Done!"
# Converting ELF to UARM
uarm: p1test
	@echo "Converting ELF to UARM..."
	$(UC) -k p1test

# Linking
p1test: p1test.o pcb.o asl.o
	@echo "Linking..."
	$(LD) -T $(LDSCRIPTS) -o p1test $(CRTSO) $(LIBUARM) p1test.o pcb.o asl.o

# Compiling
p1test.o: $(P1TEST)
	@echo "Compiling p1test..."
	$(CC) $(CFLAGS) p1test.o $(P1TEST)

pcb.o: $(PCB)
	@echo "Compiling PCB..."
	$(CC) $(CFLAGS) pcb.o $(PCB)

asl.o: $(ASL)
	@echo "Compiling ASL..."
	$(CC) $(CFLAGS) asl.o $(ASL)

# [3] CLEANING
clean:
	rm -rf *o all
