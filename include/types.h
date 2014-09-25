#ifndef TYPES_H
#define TYPES_H

#include "/usr/include/uarm/uARMtypes.h"
#include "base.h"
#include "const.h"

/* process control block type */

/* Process Control Block type */
typedef struct pcb_t
{
	struct pcb_t
	/* Process queue fields */
		*p_next,			/**< Pointer to next entry */

	/* Process tree fields */
		*p_prnt,			/**< Pointer to parent */
		*p_child,			/**< Pointer to first child */
		*p_sib;				/**< Pointer to next sibling */

	/* Process management */
	state_t p_s;								/**< Processor state */
	S32 *p_semAdd;								/**< Pointer to semaphore on which process blocked */
	cpu_t p_cpu_time;								/**< Process CPU time */
	U32 exceptionState[NUM_EXCEPTIONS];			/**< Exception State Vector */
	state_t *p_stateOldArea[NUM_EXCEPTIONS];	/**< Old processor states, one for each exception type */
	state_t *p_stateNewArea[NUM_EXCEPTIONS];	/**< New processor states, one for each exception type */
	U32 p_isBlocked;							/**< Semaphore Flag */
} pcb_t;

/* Semaphore Descriptor type */
typedef struct semd_t
{
	struct semd_t *s_next; 		/**<  next element on the ASL */
	int *s_semdAdd; 			/**<  pointer to the semaphore */
	pcb_t *s_procQ; 			/**<  tail pointer to a process queue */
} semd_t;

/* Device semaphores */
typedef struct
{
	int disk[DEV_PER_INT];
	int tape[DEV_PER_INT];
	int network[DEV_PER_INT];
	int printer[DEV_PER_INT];
	int terminalR[DEV_PER_INT];
	int terminalT[DEV_PER_INT];
} DeviceSemaphores;
#endif

