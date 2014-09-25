/* Semaphore list handling functions */

/* Dependencies */
#include "../h/asl.h"

/* Internal function declarations */
HIDDEN int isEmpty(semd_t *header);
HIDDEN void addToASL(semd_t *sem);
HIDDEN semd_t *removeFromSemdFree(void);
HIDDEN semd_t *findSemaphore(int *semAdd);
HIDDEN void freeSemaphore(semd_t *sem);

/* Pointer to the head of the Active Semaphore List (ASL) */
HIDDEN semd_t *semd_h;

/* Pointer to the head of the unused semaphore descriptors list */
HIDDEN semd_t *semdFree_h;

/*
 * Check if a list is empty
 * Input:   dummy header of a list
 * Output:  TRUE, if the list is empty
 *          FALSE, else
*/
HIDDEN int isEmpty(semd_t *header)
{
	return !header->s_next;
}

/*
 * Add a semaphore to ASL
 * Input:   semd_t *sem, pointer to the semaphore
 * Output:  void
*/
HIDDEN void addToASL(semd_t *sem)
{
	semd_t *it;

	/* Iterate ASL until:
	  	the end of ASL is reached OR
	  	a semaphore with larger semAdd is found */
	for (it = semd_h;
		 it->s_next && it->s_next->s_semAdd < sem->s_semAdd;
		 it = it->s_next);

	sem->s_next = it->s_next;
	it->s_next = sem;
}

/*
 * Remove a semaphore from the top of semdFree
 * Input:   void
 * Output:  semd_t *,   pointer to a free semaphore, if semdFree is not empty
 *                      NULL, else
*/
HIDDEN semd_t *removeFromSemdFree(void)
{
	semd_t *output;

	/* Pre-conditions: semdFree is not empty */
	if (isEmpty(semdFree_h)) return NULL;

	output = semdFree_h->s_next;
	semdFree_h->s_next = semdFree_h->s_next->s_next;
	output->s_next = NULL;
	output->s_procQ = mkEmptyProcQ();

	return output;
}

/*
 * Search a semaphore in ASL
 * Input:	int *semAdd, 	physical address of the semaphore
 * Output:	semd_t *,		pointer to the previous element
*/
HIDDEN semd_t *findSemaphore(int *semAdd)
{
	semd_t *it;

	/* Iterate ASL until:
	  	the end of ASL is reached OR
	  	a semaphore with >= semAdd is found */
	for (it = semd_h;
		 it->s_next && it->s_next->s_semAdd < semAdd;
		 it = it->s_next);

	return (it->s_next && it->s_next->s_semAdd == semAdd)? it : NULL;
}

/*
 * Remove a semaphore from ASL. Add it to semdFree list.
 * Input: semd_t *sem, pointer to the previous semaphore (or dummy node)
*/
HIDDEN void freeSemaphore(semd_t *sem)
{
	semd_t *tmp;

	tmp = sem->s_next->s_next;

	/* Add semAdd to semdFree */
	sem->s_next->s_next = semdFree_h->s_next;
	semdFree_h->s_next = sem->s_next;

	/* Remove semAdd from ASL */
	sem->s_next = tmp;
}

/*
 * Initialize the semdFree list to contain all the elements of the
 * array static semd_t semdTable[MAXPROC + 2].
 * The size is increased by 2 because of 2 dummy headers (one for
 * semdFree and one for ASL).
 * This method will be only called once during data structure
 * initialization.
 *
 * Computational Cost := O(n) : n = "max number of semaphors"
*/
EXTERN void initASL(void)
{
	static semd_t semdTable[MAXPROC + 2];
	int i;

	for (i = 0; i < MAXPROC; i++)
		semdTable[i].s_next = &semdTable[i + 1];
	semdTable[MAXPROC].s_next = NULL;

	/* semdTable[0] is the dummy header for semdFree */
	semdFree_h = &semdTable[0];

	/* semdTable[MAXPROC + 1] is the dummy header for ASL */
	semdTable[MAXPROC + 1].s_next = NULL;
	semd_h = &semdTable[MAXPROC + 1];

	semd_h->s_next = NULL;
	semd_h->s_semAdd = NULL;
	semd_h->s_procQ = mkEmptyProcQ();
}

/*
 * Insert the ProcBlk pointed to by p at the tail of the process queue
 * associated with the semaphore whose physical address is semAdd
 * and set the semaphore address of p to semAdd.
 * If the semaphore is currently not active (i.e. there is no descriptor
 * for it in the ASL), allocate a new descriptor from the semdFree list,
 * insert it in the ASL (at the appropriate position), initialize all of
 * the fields (i.e. set s_semAdd to semAdd, and s_procq to mkEmptyProcQ()),
 * and proceed as above.
 * If a new semaphore descriptor needs to be allocated and the semdFree
 * list is empty, return TRUE.
 * In all other cases return FALSE.
 *
 * Computational Cost := O(n) : n = "max number of semaphors"
*/
EXTERN int insertBlocked(int *semAdd, pcb_t *p)
{
	int output;
	semd_t *sem;

	/* Pre-conditions: semAdd, p are not NULL */
	if (!semAdd || !p) return FALSE;

	sem = findSemaphore(semAdd);
	/* [Case 1] semAdd is in ASL */
	if (sem)
	{
		p->p_semAdd = semAdd;
	 	insertProcQ(&sem->s_next->s_procQ, p);

	 	output = FALSE;
	}
	/* [Case 2] semAdd is not in ASL */
	else
	{
		sem = removeFromSemdFree();
		/* [Case 2.1] semdFree is not empty */
		if (sem)
		{
			sem->s_semAdd = p->p_semAdd = semAdd;
			insertProcQ(&sem->s_procQ, p);
			addToASL(sem);

			output = FALSE;
		}
		/* [Case 2.2] semdFree is empty */
		else
			output = TRUE;
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
 *
 * Computational Cost := O(n) : n = "max number of semaphors"
*/
EXTERN pcb_t *removeBlocked(int *semAdd)
{
	pcb_t *output;
	semd_t *sem;

	/* Pre-conditions: semAdd is not NULL */
	if (!semAdd) return NULL;

	sem = findSemaphore(semAdd);
	/* [Case 1] semAdd is in ASL */
	if (sem)
	{
		output = removeProcQ(&sem->s_next->s_procQ);

		/* [SubCase] ProcQ is now empty */
		if (emptyProcQ(sem->s_next->s_procQ))
			/* Deallocate the semaphore */
			freeSemaphore(sem);
	}
	/* [Case 2] semAdd is not in ASL */
	else
		output = NULL;

	return output;
}

/*
 * Remove the ProcBlk pointed to by p from the process queue
 * associated with p’s semaphore (p->p_semAdd) on the ASL.
 * If ProcBlk pointed to by p does not appear in the process
 * queue associated with p’s semaphore, which is an error
 * condition, return NULL; otherwise, return p.
 *
 * Computational Cost := O(max{n, m}) : n = "max number of semaphors",
 *                                      m = "max number of concurrent processes"
*/
EXTERN pcb_t *outBlocked(pcb_t *p)
{
	pcb_t *output;
	semd_t *sem;

	/* Pre-conditions: p is not NULL */
	if (!p || !p->p_semAdd) return NULL;

	sem = findSemaphore(p->p_semAdd);

	/* [Case 1] semAdd is in ASL */
	if (sem)
	{
		output = outProcQ(&sem->s_next->s_procQ, p);

		/* [SubCase] ProcQ is now empty, deallocate the semaphore */
		if (emptyProcQ(sem->s_next->s_procQ))
			freeSemaphore(sem);
	}
	/* [Case 2] semAdd is not in ASL */
	else
		output = NULL;

	return output;
}

/*
 * Return a pointer to the ProcBlk that is at the head of the
 * process queue associated with the semaphore semAdd.
 * Return NULL if semAdd is not found on the ASL or if the
 * process queue associated with semAdd is empty.
 *
 * Computational Cost := O(n) : n = "max number of semaphors"
*/
EXTERN pcb_t *headBlocked(int *semAdd)
{
	pcb_t *output;
	semd_t *sem;

	/* Pre-conditions: semAdd is not NULL */
	if (!semAdd) return NULL;

	/* [Case 1] semAdd is in ASL */
	if ((sem = findSemaphore(semAdd)))
		output = headProcQ(sem->s_next->s_procQ);
	/* [Case 2] semAdd is not in ASL */
	else
		output = NULL;

	return output;
}
