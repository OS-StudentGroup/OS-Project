#ifndef ASL_H
#define ASL_H

/* Semaphore list handling functions */

#include "../phase1/pcb.h"
#include "../h/const.h"
#include "../h/types.h"

EXTERN void initSemd(void);
EXTERN int insertBlocked(int *semAdd, pcb_t *p);
EXTERN pcb_t *removeBlocked(int *semAdd);
EXTERN pcb_t *outBlocked(pcb_t *p);
EXTERN pcb_t *headBlocked(int *semAdd);

// Pointer to the head of the Active Semaphore List (ASL)
static semd_t *semd_h;

static semd_t *semdFree_h;

/*
 * Add items to active semaphore list
*/
HIDDEN semd_t *addToASL(semd_t *newSem, int *semAdd)
{
	semd_t *output;
	int stop = FALSE;
	semd_t *index = semd_h;
	newSema->s_semAdd = semAdd;

	// [Case 1] ASL is empty
	if(!semd_h)
	{
		semd_h = newSem;
		output = newSem;
	}
	// [Case 2] ASL is not empty
	else
	{
		// Check head
		if(index->s_semAdd > semAdd)
		{
			semd_h = newSem;
			newSem->s_next = index;
			stop = TRUE;
		}
		// Check second element
		if(!index->s_next)
		{
			index->s_next = newSem;
			stop = TRUE;
		}
		// Loop through everything but head
		while(!stop)
		{
			// If semAdd is greater than the current semAdd
			if(!index->s_next)
			{
				index->s_next = newSem;
				stop = TRUE;
			}
			else if(index->s_next->s_semAdd > semAdd)
			{
				newSem->s_next = index->s_next;
				index->s_next = newSem;
				stop = TRUE;
			}
			else
			{
				index = index->s_next;
			}
		}
		output = newSem;
	}
	return output;
}

/*
 * Return TRUE if the list is empty.
 * Return FALSE otherwise.
*/
HIDDEN int emptyList(semd_t *header)
{
	return header->s_next? FALSE : TRUE;
}

/*
 * Looks through list for semAdd.
 * If not found return NULL
*/
HIDDEN semd_t *findActive(int *semAdd)
{
	semd_t *output;
	// [Case 1] ASL is empty
	if(emptyList(semd_h))
	{
		output = NULL;
	}
	// [Case 2] ASL is not empty
	else
	{
		semd_t *it;
		for(it = semd_h->s_next;
			it && it->s_semAdd != semAdd;
			it = it->s_next);
		// [Case 1.1] semAdd is in ASL
		if(it->s_semAdd == semAdd)
		{
			output = it;
		}
		// [Case 1.2] semAdd is not in ASL
		else
		{
			output = NULL;
		}
	}
	return output;
}

/*
 * Looks through list for semAdd if not found allocNewASL
*/
HIDDEN semd_t *removeActive(int *semAdd)
{
	semd_t *output;
	semd_t *index = semd_h;
	semd_t *deletedNode;
	// [Case 1] semAdd is the head
	if(semd_h->s_semAdd == semAdd)
	{
		deletedNode = semd_h;
		// [Case 1.1] 1 semaphore
		if(!semd_h->s_next)
		{
			semd_h = NULL;
		}
		// [Case 1.1] >1 semaphores
		else
		{
			semd_h = semd_h->s_next;
		}
		deletedNode->s_next= NULL;
		output = deletedNode;
	}
	// [Case 2] semAdd is not the head
	else
	{
		if(semd_h->s_next->s_semAdd == semAdd)
		{
			deletedNode = semd_h->s_next;
			if(!semd_h->s_next->s_next)
			{
				semd_h->s_next = NULL;
			}
			else
			{
				semd_h->s_next = semd_h->s_next->s_next;
			}
			deletedNode->s_next= NULL;
			output = deletedNode;
		}
		else
		{
			while(index->s_next != NULL)
			{
				if(index->s_next->s_semAdd == semAdd)
				{
					if(index->s_next->s_next)
					{
						deletedNode = index->s_next;
						index->s_next = index->s_next->s_next;
						deletedNode->s_next = NULL;
						output = deletedNode;
					}
					else
					{
						deletedNode = index->s_next;
						index->s_next = NULL;
						deletedNode->s_next = NULL;
						output = deletedNode;
					}
				}
				else
				{
					index = index->s_next;
				}
			}
		}
	}
	return output;
}

/*
 * Removes the top of semdFree
*/
HIDDEN semd_t *removeFree()
{
	semd_t *output;
	// [Case 1] semdFree is empty
	if(emptyList(semdFree_h))
	{
		output = NULL;
	}
	// [Case 2] semdFree is not empty
	else
	{
		semd_t *sem = semdFree_h->s_next;
		// [Case 2.1] 1 unused semaphore
		if(!semdFree_h->s_next->s_next)
		{
			semdFree_h->s_next = NULL;
		}
		// [Case 2.2] >1 unused semaphores
		else
		{
			semdFree_h->s_next = semdFree_h->s_next->s_next;
		}
		sem->s_next = NULL;
		sem->s_procQ = mkEmptyProcQ();
		output = sem;
	}
	return output;
}

/*
 * Add the top of the Free list
*/
HIDDEN void addFree(semd_t *newSema)
{
	if(emptyList(semdFree_h))
	{
		semdFree_h = newSema;
	}
	else
	{
		newSema->s_next = semdFree_h;
		semdFree_h = newSema;
	}
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
	semd_t *sem = findActive(semAdd);
	// [Case 1] semAdd is active
	if(sem)
	{
	 	p->p_semAdd = semAdd;
	 	insertProcQ(&(sem->s_procQ), p);
	 	output = FALSE;
	}
	// [Case 2] semAdd is not active
	else
	{
		sem = removeFree();
		// [Case 1.1] semdFree is not empty
		if(sem)
		{
			sem = addToASL(sem, semAdd);
			output = FALSE;
		}
		// [Case 1.2] semdFree is not empty
		else
		{
			output = TRUE;
		}
	}
	return output;
}

/*
 * Search the ASL for a descriptor of this semaphore. If none is
 * found, return NULL; otherwise, remove the first (i.e. head) ProcBlk
 * from the process queue of the found semaphore descriptor and return
 * a pointer to it. If the process queue for this semaphore becomes
 * empty (emptyProcQ(s procq) is TRUE), remove the semaphore
 * descriptor from the ASL and return it to the semdFree list.
*/
EXTERN pcb_t *removeBlocked(int *semAdd)
{
	pcb_t *output;
	semd_t *sem = findActive(semAdd);
	// [Case 1] semaphore is in ASL
	if(sem)
	{
		pcb_t *proc = removeProcQ(&(sem->s_procQ));
		if(emptyProcQ(sem->s_procQ))
		{
			sem = removeActive(semAdd);
			addFree(sem);
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
	semd_t *sem = findActive(p->p_semAdd);
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
	semd_t *sem = findActive(semAdd);
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
