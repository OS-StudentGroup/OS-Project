// Types definitions
#ifndef TYPES_H
#define TYPES_H

// temporary definition for phase1
typedef void *state_t;

// Process Control Block type
typedef struct pcb_t
{
	/* Process queue fields */
	struct pcb_t
		*p_next,		/**< Pointer to next entry */

	/* Process tree fields */
		*p_prnt,		/**< Pointer to parent */
		*p_child,		/**< Pointer to first child */
		*p_sib;			/**< Pointer to next sibling */

	/* Process management */
	state_t p_s;		/**< Processor state */
	S32 *p_semAdd;		/**< Pointer to semaphore on which process blocked */
	int p_pid;			/**< Process id */
	cpu_t p_cpu_time;	/**< Process CPU time */
} pcb_t;

#endif
