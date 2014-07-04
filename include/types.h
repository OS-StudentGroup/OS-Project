// Types definitions
#ifndef TYPES_H
#define TYPES_H

#include "base.h"
#include "const.h"
#include "uARMconst.h"
#include "uARMtypes.h"

// Process Control Block type
typedef struct pcb_t
{
	/* Process queue fields */
	struct pcb_t
		*p_next,					/**< Pointer to next entry */

	/* Process tree fields */
		*p_prnt,					/**< Pointer to parent */
		*p_child,					/**< Pointer to first child */
		*p_sib;						/**< Pointer to next sibling */

	/* Process management */
	state_t p_s;					/**< Processor state */
	S32 *p_semAdd;					/**< Pointer to semaphore on which process blocked */
	int p_pid;						/**< Process id */
	cpu_t p_cpu_time;					/**< Process CPU time */
	int ExStVec[MAX_STATE_VECTOR];	/**< Exception State Vector */
	state_t *tlbState_old;			/**< TLB old area */
	state_t *tlbState_new;			/**< TLB new area */
	state_t *pgmtrapState_old;		/**< PgmTrap old area */
	state_t *pgmtrapState_new;		/**< PgmTrap new area */
	state_t *sysbpState_old;		/**< SysBP old area */
	state_t *sysbpState_new;		/**< SysBP new area */
	int p_isOnDev;					/**< Semaphore Flag */
} pcb_t;

/* Used PCBs */
typedef struct pcb_pid_t
{
	int pid;		/**< Process PID */
	pcb_t *pcb;		/**< Pointer to PCB */
} pcb_pid_t;

// Semaphore Descriptor type
typedef struct semd_t
{
	struct semd_t *s_next; 		// next element on the ASL
	int *s_semAdd; 				// pointer to the semaphore
	pcb_t *s_procQ; 			// tail pointer to a process queue
} semd_t;

#endif
