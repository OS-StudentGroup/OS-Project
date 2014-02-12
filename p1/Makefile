# MACRO DEFINITIONS
## COMMANDS
### [C compiler]
CC = arm-none-eabi-gcc
### [C compiler flags]
CFLAGS = -mcpu=arm7tdmi -c -o
### [LD linker]
LD = arm-none-eabi-ld
### [UARM converter]
UC = elf2uarm

## ADDRESSES
### [UARM library]
LIBUARM = /usr/include/uarm/libuarm.o
### [LD linker scripts]
LDSCRIPTS = /usr/include/uarm/ldscripts/elf32ltsarm.h.uarmcore.x
### [C run-time start-off]
CRTSO = /usr/include/uarm/crtso.o
### [p1test.c]
P1TEST = p1test.c

# RULE DEFINITIONS
all: uarm
uarm: p1test
	$(UC) -k p1test
p1test: p1test.o
	$(LD) -T $(LDSCRIPTS) -o p1test $(CRTSO) $(LIBUARM) p1test.o
p1test.o: $(P1TEST)
	$(CC) $(CFLAGS) p1test.o $(P1TEST)

# CLEANING
clean:
	rm -rf *o all