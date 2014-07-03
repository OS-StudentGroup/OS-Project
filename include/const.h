/**
@file const.h
@author
@note This header file contains the global constant & macro definitions.
*/
#ifndef CONST_H
#define CONST_H

/* Bus register area */
#define BUS_BASEADDRESS 0x00000000

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

#endif
