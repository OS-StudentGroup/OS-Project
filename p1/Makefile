# [1] MACRO DEFINITIONS
# [1.1] Commands
# Cross C compiler
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
# p1test.c
P1TEST = p1test.c
# pcb.c
PCB = ../c/pcb.c
# asl.c
ASL = ../c/asl.c

# [2] RULE DEFINITIONS
# Main target
all: uarm
# Converting
uarm: p1test
	$(UC) -k p1test
# Linking
p1test: p1test.o pcb.o asl.o
	$(LD) -T $(LDSCRIPTS) -o p1test $(CRTSO) $(LIBUARM) p1test.o pcb.o asl.o
# Compiling
p1test.o: $(P1TEST)
	$(CC) $(CFLAGS) p1test.o $(P1TEST)

pcb.o: $(PCB)
	$(CC) $(CFLAGS) pcb.o $(PCB)

asl.o: $(ASL)
	$(CC) $(CFLAGS) asl.o $(ASL)

# [3] CLEANING
clean:
	rm -rf *o all