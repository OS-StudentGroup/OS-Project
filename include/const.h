/**
@file const.h
@note This header file contains the global constant & macro definitions.
*/
#ifndef CONST_H
#define CONST_H

/*****************************************************/
/* Exception State Vector */
#define NUM_EXCEPTIONS 3
#define TLB_EXCEPTION 0
#define PROGRAM_TRAP_EXCEPTION 1
#define SYS_BK_EXCEPTION 2

/* Status Word Table */
#define STATUS_WORD_ROWS 6
#define STATUS_WORD_COLS 8

/*****************************************************/
/* max number of overall concurrent processes */
#define MAXPROC 	20

/* number of usermode processes (not including master process and system daemons) */
#define UPROCMAX 	3

/* general purpose constants */
#define EXTERN 		extern
#define HIDDEN 		static
#define FALSE		0
#define TRUE 		1
#define NULL 		((void *)0)

/* Device Check Addresses */
#define DEV_CHECK_ADDRESS_0 1
#define DEV_CHECK_ADDRESS_1 2
#define DEV_CHECK_ADDRESS_2 4
#define DEV_CHECK_ADDRESS_3 8
#define DEV_CHECK_ADDRESS_4 16
#define DEV_CHECK_ADDRESS_5 32
#define DEV_CHECK_ADDRESS_6 64
#define DEV_CHECK_ADDRESS_7 128

/* Interrupt Line Numbers */
#define IL_IPI 0
#define IL_CPUTIMER 1
#define IL_TIMER 2
#define IL_DISK 3
#define IL_TAPE 4
#define IL_ETHERNET 5
#define IL_PRINTER 6
#define IL_TERMINAL 7

/* Check Status Bit */
#define CHECK_STATUS_BIT 0xFF

#define WORD_SIZE 4

#define DEV_DIFF 3

/* Check Bit */
#define CHECK_FIFTH_BIT 	0x10
#define CHECK_EIGHTH_BIT 	0x80

/*Device register words*/
#define DEV_REG_STATUS        0x0
#define DEV_REG_COMMAND       0x4
#define DEV_REG_DATA0         0x8
#define DEV_REG_DATA1         0xC

#define TERM_R_STATUS     0x0
#define TERM_R_COMMAND    0x4
#define TERM_T_STATUS     0x8
#define TERM_T_COMMAND    0xC
#endif
