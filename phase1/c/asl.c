/* Semaphore list handling functions */

/* Dependencies */
#include "../e/pcb.e"

/* Internal function declarations */
HIDDEN int isEmpty(semd_t *header);
HIDDEN void addToASL(semd_t *sem);
HIDDEN semd_t *removeFromSemdFree(void);
HIDDEN semd_t *findSemaphore(int *semAdd);
HIDDEN void freeSemaphore(semd_t *sem);

/* Pointer to the head of the Active Semaphore List (ASL) */
HIDDEN semd_t *semd_h = NULL;

/* Pointer to the head of the unused semaphore descriptors list */
HIDDEN semd_t *semdFree_h = NULL;

/**
@brief Check if a list is empty.
@param Dummy header of a list.
@return  TRUE, if the list is empty, otherwise FALSE.
*/
HIDDEN int isEmpty(semd_t *header)
{
	return !header->s_next;
}

/**
@brief Add a semaphore to ASL.
@param sem Pointer to the semaphore.
@return Void.
*/
HIDDEN void addToASL(semd_t *sem)
{
	semd_t *it;

	/* Iterate ASL until: the end of ASL is reached OR a semaphore with larger semAdd is found */
	for (it = semd_h; it->s_next && it->s_next->s_semdAdd < sem->s_semdAdd; it = it->s_next);

	sem->s_next = it->s_next;
	it->s_next = sem;
}

/**
@brief Remove a semaphore from the top of semdFree.
@param sem Pointer to the semaphore.
@return The pointer to a free semaphore, if semdFree is not empty, otherwise NULL.
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
	output->s_semdAdd = NULL;

	return output;
}

/**
@brief Search for a semaphore in ASL.
@param semAdd Address of the semaphore.
@return The pointer to the previous element, otherwise NULL.
*/
HIDDEN semd_t *findSemaphore(int *semAdd)
{
	semd_t *it;

	/* Iterate ASL until: the end of ASL is reached OR a semaphore with >= semAdd is found */
	for (it = semd_h; it->s_next && it->s_next->s_semdAdd < semAdd; it = it->s_next);

	return (it->s_next && it->s_next->s_semdAdd == semAdd)? it : NULL;
}

/**
@brief Remove a semaphore from ASL. Add it to semdFree list.
@param sem Pointer to the previous semaphore (or dummy node).
@return Void.
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

/**
@brief Initialize the semdFree list to contain all the elements of the
array static semd_t semdTable[MAXPROC + 2].
The size is increased by 2 because of 2 dummy headers (one for
semdFree and one for ASL).
This method will be only called once during data structure initialization.
@return Void.
*/
EXTERN void initASL(void)
{
	static semd_t semdTable[MAXPROC + 2];
	int i;

	for (i = 0; i < MAXPROC; i++) semdTable[i].s_next = &semdTable[i + 1];
	semdTable[MAXPROC].s_next = NULL;

	/* semdTable[0] is the dummy header for semdFree */
	semdFree_h = &semdTable[0];

	/* semdTable[MAXPROC + 1] is the dummy header for ASL */
	semdTable[MAXPROC + 1].s_next = NULL;
	semd_h = &semdTable[MAXPROC + 1];

	semd_h->s_next = NULL;
	semd_h->s_semdAdd = NULL;
	semd_h->s_procQ = mkEmptyProcQ();
}

/**
@brief Insert the ProcBlk pointed to by p at the tail of the process queue
associated with the semaphore whose physical address is semAdd and set the
semaphore address of p to semAdd.
If the semaphore is currently not active (i.e. there is no descriptor
for it in the ASL), allocate a new descriptor from the semdFree list,
insert it in the ASL (at the appropriate position), initialize all of
the fields (i.e. set s_semdAdd to semAdd, and s_procq to mkEmptyProcQ()),
and proceed as above.
@param semAdd Pointer to the semaphore.
@param p Pointer to the ProcBlk.
@return If a new semaphore descriptor needs to be allocated and the semdFree
list is empty, return TRUE. In all other cases return FALSE.
*/
EXTERN int insertBlocked(int *semAdd, pcb_t *p)
{
	int output;
	semd_t *sem;

	/* Pre-conditions: semAdd, p are not NULL */
	if (!semAdd || !p) return FALSE;

	/* [Case 1] semAdd is in ASL */
	if ((sem = findSemaphore(semAdd)))
	{
		p->p_semAdd = semAdd;
		p->p_isBlocked = TRUE;
	 	insertProcQ(&sem->s_next->s_procQ, p);

	 	output = FALSE;
	}
	/* [Case 2] semAdd is not in ASL */
	else
	{
		/* [Case 2.1] semdFree is not empty */
		if ((sem = removeFromSemdFree()))
		{
			sem->s_semdAdd = p->p_semAdd = semAdd;
			p->p_isBlocked = TRUE;
			insertProcQ(&sem->s_procQ, p);
			addToASL(sem);

			output = FALSE;
		}
		/* [Case 2.2] semdFree is empty */
		else output = TRUE;
	}

	return output;
}

/**
@brief Search the ASL for a descriptor of this semaphore.
If none is found, return NULL; otherwise, remove the first
(i.e. head) ProcBlk from the process queue of the found
semaphore descriptor and return a pointer to it.
If the process queue for this semaphore becomes empty
(emptyProcQ(s_procq) is TRUE), remove the semaphore
descriptor from the ASL and return it to the semdFree list.
*/
EXTERN pcb_t *removeBlocked(int *semAdd)
{
	pcb_t *output;
	semd_t *sem;

	/* Pre-conditions: semAdd is not NULL */
	if (!semAdd) return NULL;

	/* [Case 1] semAdd is in ASL */
	if ((sem = findSemaphore(semAdd)))
	{
		output = removeProcQ(&sem->s_next->s_procQ);
		output->p_semAdd = NULL;
		output->p_isBlocked = FALSE;

		/* If ProcQ is now empty, deallocate the semaphore */
		if (emptyProcQ(sem->s_next->s_procQ)) freeSemaphore(sem);
	}
	/* [Case 2] semAdd is not in ASL */
	else output = NULL;

	return output;
}

/**
@brief Remove the ProcBlk pointed to by p from the process queue
associated with p’s semaphore (p->p_semAdd) on the ASL.
If ProcBlk pointed to by p does not appear in the process
queue associated with p’s semaphore, which is an error
condition, return NULL; otherwise, return p.
*/
EXTERN pcb_t *outBlocked(pcb_t *p)
{
	pcb_t *output;
	semd_t *sem;

	/* Pre-conditions: p is not NULL */
	if (!p || !p->p_semAdd) return NULL;

	/* [Case 1] semAdd is in ASL */
	if ((sem = findSemaphore(p->p_semAdd)))
	{
		output = outProcQ(&sem->s_next->s_procQ, p);
		output->p_semAdd = NULL;
		output->p_isBlocked = FALSE;

		/* If ProcQ is now empty, deallocate the semaphore */
		if (emptyProcQ(sem->s_next->s_procQ)) freeSemaphore(sem);
	}
	/* [Case 2] semAdd is not in ASL */
	else output = NULL;

	return output;
}

/**
@brief Return a pointer to the ProcBlk that is at the head of the
process queue associated with the semaphore semAdd.
Return NULL if semAdd is not found on the ASL or if the
process queue associated with semAdd is empty.
*/
EXTERN pcb_t *headBlocked(int *semAdd)
{
	semd_t *sem;

	/* Pre-conditions: semAdd is not NULL */
	if (!semAdd) return NULL;

	return ((sem = findSemaphore(semAdd)))? headProcQ(sem->s_next->s_procQ) : NULL;
}
