// Types definitions
#ifndef TYPES_H
#define TYPES_H

// temporary definition for phase1
typedef void *state_t;

// Process Control Block type
typedef struct pcb_t
{
	// process queue fields
	struct pcb_t
		*p_next, 		// pointer to next entry

	// process tree fields
		*p_prnt, 		// pointer to parent
		*p_child, 		// pointer to first child
		*p_sib; 		// pointer to next sibling
	state_t p_s; 		// processor state
	int *p_semAdd; 		// pointer to semaphore on which process blocked

	// plus other entries to be added later
} pcb_t;

#endif
