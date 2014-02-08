#ifndef ASL_H
#define ASL_H

/*
 * Semaphore list handling functions
*/

#include "../phase1/pcb.h"
#include "../h/const.h"
#include "../h/types.h"

// Function definitions
// [internal functions]
HIDDEN inline int isEmpty(semd_t *header);
HIDDEN inline void addToASL(semd_t *sem, int *semAdd);
HIDDEN inline void addToSemdFree(semd_t *sem);
HIDDEN inline semd_t *removeFromSemdFree(void);
HIDDEN inline semd_t *findSemaphore(int *semAdd);
HIDDEN inline semd_t *freeSemaphore(int *semAdd);
// [external functions]
EXTERN void initSemd(void);
EXTERN int insertBlocked(int *semAdd, pcb_t *p);
EXTERN pcb_t *removeBlocked(int *semAdd);
EXTERN pcb_t *outBlocked(pcb_t *p);
EXTERN pcb_t *headBlocked(int *semAdd);

// Pointer to the head of the Active Semaphore List (ASL)
static semd_t *semd_h;

// Pointer to the head of the unused semaphore descriptors list
static semd_t *semdFree_h;

/*
 * Check is a list is empty
 * Input:	dummy header of a list
 * Output:	TRUE, if the list is empty
 * 			FALSE, else
*/
HIDDEN inline int isEmpty(semd_t *header)
{
	return header->s_next? FALSE : TRUE;
}

/*
 * Add a semaphore to ASL
 * Input:	semd_t *sem,	pointer to the semaphore
 * 			int *semAdd,	physical address of the semaphore
 * Output:	void
*/
HIDDEN inline void addToASL(semd_t *sem, int *semAdd)
{
	semd_t *it;

	// Iterate ASL until
	for(it = semd_h;
		// the end of ASL is reached OR
		it->s_next &&
		// a semaphore with higher value is found
		it->s_next->s_semAdd < semAdd;
		it = it->s_next);

	sem->s_next = it->s_next;
	it->s_next = sem;
}

/*
 * Add a semaphore to the top of semdFree list
 * Input:	semd_t *sem, pointer to a semaphore in ASL
 * Output:	void
*/
HIDDEN inline void addToSemdFree(semd_t *sem)
{
	sem->s_next = semdFree_h->s_next;
	semdFree_h->s_next = sem;
}

/*
 * Removes a semaphore from the top of semdFree
 * Input:	void
 * Output:	semd_t *, pointer to a free semaphore
*/
HIDDEN inline semd_t *removeFromSemdFree(void)
{
	semd_t *output;
	// [Case 1] semdFree is empty
	if(isEmpty(semdFree_h))
	{
		output = NULL;
	}
	// [Case 2] semdFree is not empty
	else
	{
		output = semdFree_h->s_next;
		semdFree_h->s_next = semdFree_h->s_next->s_next;
		output->s_next = NULL;
		output->s_procQ = mkEmptyProcQ();
	}
	return output;
}

/*
 * Search a given semaphore in the ASL
 * Input:	int *semAdd, 	physical address of the semaphore
 * Output:	semd_t *,
 * 							pointer to the previous node, if the semaphore is found
 * 							NULL, else
*/
HIDDEN inline semd_t *findSemaphore(int *semAdd)
{
	semd_t *it;

	// Iterate ASL until
	for(it = semd_h;
		// the end of ASL is reached OR
		it->s_next &&
		// the semaphore is found
		it->s_next->s_semAdd != semAdd;
		it = it->s_next);

	return it;
}

/*
 * Move the semaphore from ASL to semdFree list.
 * Input:	int *semAdd,	physical address of the semaphore
 * Output:	semd_t *,		pointer to the free semaphore
*/
HIDDEN inline semd_t *freeSemaphore(int *semAdd)
{
	semd_t *output, *it;

	it = findSemaphore(semAdd);

	// [Case 1] semAdd is in ASL
	if(it->s_next)
	{
		output = it->s_next;
		output->s_next = NULL;
		addToSemdFree(output);
		it->s_next = it->s_next->s_next;
	}
	// [Case 2] semAdd is not in ASL
	else
	{
		output = NULL;
	}
	return output;
}

/*
 * Insert the ProcBlk pointed to by p at the tail of the process queue
 * associated with the semaphore whose physical address is semAdd
 * and set the semaphore address of p to semAdd.
 * If the semaphore is currently not active (i.e. there is no descriptor
 * for it in the ASL), allocate a new descriptor from the semdFree list,
 * insert it in the ASL (at the appropriate position), initialize all of
 * the fields (i.e. set s semAdd to semAdd, and s procq to mkEmptyProcQ()),
 * and proceed as above.
 * If a new semaphore descriptor needs to be allocated and the semdFree
 * list is empty, return TRUE.
 * In all other cases return FALSE.
*/
EXTERN int insertBlocked(int *semAdd, pcb_t *p);
{
	int output;
	semd_t *sem = findSemaphore(semAdd);
	// [Case 1] semAdd is active
	if(sem->s_next)
	{
	 	p->p_semAdd = semAdd;
	 	insertProcQ(&(sem->s_next->s_procQ), p);
	 	output = FALSE;
	}
	// [Case 2] semAdd is not active
	else
	{
		sem = removeFromSemdFree();
		// [Case 2.1] semdFree is not empty
		if(sem)
		{
			addToASL(sem, semAdd);
			output = FALSE;
		}
		// [Case 2.2] semdFree is empty
		else
		{
			output = TRUE;
		}
	}
	return output;
}

/*
 * Search the ASL for a descriptor of this semaphore.
 * If none is found, return NULL; otherwise, remove the first
 * (i.e. head) ProcBlk from the process queue of the found
 * semaphore descriptor and return a pointer to it.
 * If the process queue for this semaphore becomes empty
 * (emptyProcQ(s_procq) is TRUE), remove the semaphore
 * descriptor from the ASL and return it to the semdFree list.
*/
EXTERN pcb_t *removeBlocked(int *semAdd)
{
	pcb_t *output;
	semd_t *sem = findSemaphore(semAdd);
	// [Case 1] semaphore is in ASL
	if(sem)
	{
		pcb_t *proc = removeProcQ(&(sem->s_procQ));
		if(emptyProcQ(sem->s_procQ))
		{
			sem = freeSemaphore(semAdd);
		}
		output = proc;
	}
	// [Case 2] semaphore is not in ASL
	else
	{
		output = NULL;
	}
}

/*
 * Remove the ProcBlk pointed to by p from the process queue
 * associated with p’s semaphore (p->p_semAdd) on the ASL.
 * If ProcBlk pointed to by p does not appear in the process
 * queue associated with p’s semaphore, which is an error
 * condition, return NULL; otherwise, return p.
*/
EXTERN pcb_t *outBlocked(pcb_t *p);
{
	pcb_t *output;
	semd_t *sem = findSemaphore(p->p_semAdd);
	// [Case 1] ProcBlk is in the semaphore
	if(sem)
	{
		output = outProcQ(&(sem->s_procQ), p);
	}
	// [Case 2] ProcBlk is not in the semaphore
	else
	{
		output = NULL;
	}
	return output;
}

/*
 * Return a pointer to the ProcBlk that is at the head of the
 * process queue associated with the semaphore semAdd.
 * Return NULL if semAdd is not found on the ASL or if the
 * process queue associated with semAdd is empty.
*/
EXTERN pcb_t *headBlocked(int *semAdd);
{
	pcb_t *output;
	semd_t *sem = findSemaphore(semAdd);
	// [Case 1] semAdd is in ASL
	if(sem)
	{
		output = headProcQ(sema->s_procQ);
	}
	// [Case 2] semAdd is not in ASL
	else
	{
		output = NULL;
	}
	return output;
}

/*
 * Initialize the ASL
*/
HIDDEN void initASL()
{
	static semd_t semdTable[MAXPROC];
	int i;
	for(i = 0; i < (MAXPROC-1); i++)
	{
		semdTable[i].s_next = &semdTable[i + 1];
	}
	semdTable[(MAXPROC-1)].s_next = NULL;
	semdFree_h = &semdTable[0];
	semd_h = NULL;
}
#endif
