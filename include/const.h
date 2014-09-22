/**
@file const.h
@author
@note This header file contains the global constant & macro definitions.
*/
#ifndef CONST_H
#define CONST_H
/*****************************************************/
/* Exception State Vector */
#define MAX_STATE_VECTOR 3
#define ESV_TLB 0
#define ESV_PGMTRAP 1
#define ESV_SYSBP 2

/* isOnDev State */
#define IS_ON_DEV 1
#define IS_ON_PSEUDO 2
#define IS_ON_SEM 3

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

/* Device Check Address */

#define DEV_CHECK_ADDRESS_0 0x40
#define DEV_CHECK_ADDRESS_1 0x80
#define DEV_CHECK_ADDRESS_2 0x20
#define DEV_CHECK_ADDRESS_3 0x6FE0
#define DEV_CHECK_ADDRESS_4 0x6FE4
#define DEV_CHECK_ADDRESS_5 0x6FE8
#define DEV_CHECK_ADDRESS_6 0x6FEC
#define DEV_CHECK_ADDRESS_7 0x6FF0

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
