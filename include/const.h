/**
@file const.h
@note This header file contains the global constant & macro definitions.
*/
#ifndef CONST_H
#define CONST_H

/* Max number of overall concurrent processes */
#define MAXPROC 	20

/* Number of User Mode processes (not including master process and system daemons) */
#define UPROCMAX 	3

/* Number of exception types */
#define NUM_EXCEPTIONS 3

/* Word Size */
#define WORD_SIZE 4

/* SYS5 mnemonic constant */
#define SPECPGMT 0
#define SPECTLB 1
#define SPECSYSBP 2
#define SPECTRAPVEC 5

/* General purpose constants */
#define EXTERN extern
#define HIDDEN static
#define FALSE 0
#define TRUE 1
#define NULL ((void *)0)

/* Exception State Vector */
#define TLB_EXCEPTION 0
#define PGMTRAP_EXCEPTION 1
#define SYSBK_EXCEPTION 2

/* Device Check Addresses */
#define DEV_CHECK_ADDRESS_0 1
#define DEV_CHECK_ADDRESS_1 2
#define DEV_CHECK_ADDRESS_2 4
#define DEV_CHECK_ADDRESS_3 8
#define DEV_CHECK_ADDRESS_4 16
#define DEV_CHECK_ADDRESS_5 32
#define DEV_CHECK_ADDRESS_6 64
#define DEV_CHECK_ADDRESS_7 128

#define DEV_PER_INT 8 /* Maximum number of devices per interrupt line */

/* Device Check Line */
#define DEV_CHECK_LINE_0 0
#define DEV_CHECK_LINE_1 1
#define DEV_CHECK_LINE_2 2
#define DEV_CHECK_LINE_3 3
#define DEV_CHECK_LINE_4 4
#define DEV_CHECK_LINE_5 5
#define DEV_CHECK_LINE_6 6
#define DEV_CHECK_LINE_7 7

#endif
