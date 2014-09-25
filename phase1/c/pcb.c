/* PCB handling functions */

/* Dependencies */
#include "../h/pcb.h"

/* Internal function declarations */
HIDDEN int hasOneProcBlk(pcb_t *tp);

/* Pointer to the head of the pcbFree list */
HIDDEN pcb_t *pcbFree_h;

/*
 * Check if a ProcQ has only one ProcBlk
 * Input:   tail-pointer of a non-empty ProcQ
 * Output:  TRUE, if ProcQ has only one ProcBlk
 *          FALSE, else
*/
HIDDEN int hasOneProcBlk(pcb_t *tp)
{
	return tp->p_next == tp;
}

/*
 * Insert the element pointed to by p onto the pcbFree list.
 *
 * Computational Cost := O(1)
*/
EXTERN void freePcb(pcb_t *p)
{
	insertProcQ(&pcbFree_h, p);
}

/*
 * Return NULL if the pcbFree list is empty.
 * Otherwise, remove an element from the pcbFree list, provide
 * initial values for ALL of the ProcBlk’s ﬁelds (i.e. NULL and/or 0)
 * and then return a pointer to the removed element.
 * ProcBlk’s get reused, so it is important that no previous value
 * persist in a ProcBlk when it gets reallocated.
 *
 * Computational Cost := O(1)
*/
EXTERN pcb_t *allocPcb(void)
{
	pcb_t *output;

	/* If pcbFree is not empty */
	if ((output = removeProcQ(&pcbFree_h)))
	{
		int i;
		output->p_next = output->p_prnt = output->p_child = output->p_sib = NULL;
		output->p_semAdd = NULL;
		output->p_isBlocked = FALSE;
		for (i = 0; i < 3; i++)
		{
			output->exceptionState[i] = 0;
			output->p_stateOldArea[i] = output->p_stateNewArea[i] = NULL;
		}
	}

	return output;
}

/*
 * Initialize the pcbFree list to contain all the elements of the
 * static array of MAXPROC ProcBlk’s.
 * This method shall be called only once during data structure initialization.
 *
 * Computational Cost := O(n) : n = "max number of concurrent processes"
*/
EXTERN void initPcbs(void)
{
	static pcb_t pcbs[MAXPROC];
	int i;

	pcbFree_h = mkEmptyProcQ();
	for (i = 0; i < MAXPROC; i++)
		insertProcQ(&pcbFree_h, &pcbs[i]);
}

/*
 * This method is used to initialize a variable to be tail pointer to a
 * process queue.
 * Return a pointer to the tail of an empty process queue;
 * i.e. NULL.
 *
 * Computational Cost := O(1)
*/
EXTERN pcb_t *mkEmptyProcQ(void)
{
	return NULL;
}

/*
 * Return TRUE if the queue whose tail is pointed to by tp is empty.
 * Return FALSE otherwise.
 *
 * Computational Cost := O(1)
*/
EXTERN int emptyProcQ(pcb_t *tp)
{
	return !tp;
}

/*
 * Insert the ProcBlk pointed to by p into the process queue whose
 * tail-pointer is pointed to by tp.
 * Note the double indirection through tp to allow for the possible
 * updating of the tail pointer as well.
 *
 * Computational Cost := O(1)
*/
EXTERN void insertProcQ(pcb_t **tp, pcb_t *p)
{
	/* Pre-conditions: p is not NULL */
	if (!p) return;

	/* [Case 1] ProcQ is empty */
	if (emptyProcQ(*tp))
	{
		*tp = p;
		(*tp)->p_next = *tp;
	}
	/* [Case 2] ProcQ is not empty */
	else
	{
		p->p_next = (*tp)->p_next;
		(*tp)->p_next = p;
		*tp = p;
	}
}

/*
 * Remove the first (i.e. head) element from the process queue whose
 * tail-pointer is pointed to by tp.
 * Return NULL if the process queue was initially empty; otherwise
 * return the pointer to the removed element.
 * Update the process queue’s tail pointer if necessary.
 *
 * Computational Cost := O(1)
*/
EXTERN pcb_t *removeProcQ(pcb_t **tp)
{
	pcb_t *output;

	/* Pre-conditions: ProcQ is not empty */
	if (emptyProcQ(*tp)) return NULL;

	output = (*tp)->p_next;

	/* [Case 1] ProcQ has 1 ProcBlk */
	if (hasOneProcBlk(*tp))
		*tp = mkEmptyProcQ();
	/* [Case 2] ProcQ has >1 ProcBlk */
	else
		(*tp)->p_next = (*tp)->p_next->p_next;

	return output;
}

/*
 * Remove the ProcBlk pointed to by p from the process queue
 * whose tail-pointer is pointed to by tp.
 * Update the process queue’s tail pointer if necessary.
 * If the desired entry is not in the indicated queue
 * (an error condition), return NULL; otherwise, return p.
 * Note that p can point to any element of the process queue.
 *
 * Computational Cost := O(n) : n = "max number of concurrent processes"
 */
EXTERN pcb_t *outProcQ(pcb_t **tp, pcb_t *p)
{
	pcb_t *output, *it;

	/* Pre-conditions: ProcQ is not empty and p is not NULL */
	if (emptyProcQ(*tp) || !p) return NULL;

	/* [Case 1] ProcQ has 1 ProcBlk */
	if (hasOneProcBlk(*tp))
	{
		/* [Case 1.1] p is in ProcQ */
		if (*tp == p)
		{
			output = p;
			*tp = mkEmptyProcQ();
		}
		/* [Case 1.2] p is not in ProcQ */
		else
			output = NULL;
	}
	/* [Case 2] ProcQ has >1 ProcBlk */
	else
	{
		/* Iterate PCB until:
			the end is reached OR
			the ProcBlk is found */
		for (it = (*tp)->p_next;
			 it != *tp && it->p_next != p;
			 it = it->p_next);

		/* [Case 2.1] p is in ProcQ */
		if (it->p_next == p)
		{
			output = p;

			/* [SubCase] p is in the tail */
			if (it->p_next == *tp)
				/* Update the tail-pointer */
				*tp = it;

			it->p_next = it->p_next->p_next;
		}
		/* [Case 2.2] p is not in ProcQ */
		else
			output = NULL;
	}

	return output;
}

/*
 * Return a pointer to the first ProcBlk from the process queue whose
 * tail is pointed to by tp.
 * Do not remove this ProcBlk from the process queue.
 * Return NULL if the process queue is empty.
 *
 * Computational Cost := O(1)
*/
EXTERN pcb_t *headProcQ(pcb_t *tp)
{
	return emptyProcQ(tp)? NULL : tp->p_next;
}

/*
 * Return TRUE if the ProcBlk pointed to by p has no children.
 * Return FALSE otherwise.
 *
 * Computational Cost := O(1)
*/
EXTERN int emptyChild(pcb_t *p)
{
	return (!p)? TRUE : !p->p_child;
}

/*
 * Make the ProcBlk pointed to by p a child of the ProcBlk
 * pointed to by prnt.
 *
 * Computational Cost := O(n) : n = "max number of concurrent processes"
*/
EXTERN void insertChild(pcb_t *prnt, pcb_t *p)
{
	/* Pre-conditions: prnt and p are not NULL */
	if (!prnt || !p) return;

	p->p_sib = prnt->p_child;
	p->p_prnt = prnt;
	prnt->p_child = p;
}

/*
 * Make the first child of the ProcBlk pointed to by p no
 * longer a child of p.
 * Return NULL if initially there were no children of p.
 * Otherwise, return a pointer to this removed first child ProcBlk.
 *
 * Computational Cost := O(1)
*/
EXTERN pcb_t *removeChild(pcb_t *p)
{
	pcb_t *output;

	/* Pre-conditions: p has children */
	if (emptyChild(p)) return NULL;

	output = p->p_child;
	p->p_child = p->p_child->p_sib;

	return output;
}

/*
 * Make the ProcBlk pointed to by p no longer the child
 * of its parent.
 * If the ProcBlk pointed to by p has no parent, return NULL;
 * otherwise, return p.
 * Note that the element pointed to by p need not be the first
 * child of its parent.
 *
 * Computational Cost := O(n) : n = "max number of concurrent processes"
*/
EXTERN pcb_t *outChild(pcb_t *p)
{
	pcb_t *it;

	/* Pre-conditions: p is not NULL and has parent */
	if (!p || !p->p_prnt) return NULL;

	/* [Case 1] p is the first child */
	if (p->p_prnt->p_child == p)
		p->p_prnt->p_child = p->p_sib;
	/* [Case 2] p is not the first child */
	else
	{
		/* Iterate the children until the child before p is reached */
		for (it = p->p_prnt->p_child; it->p_sib != p; it = it->p_sib);

		it->p_sib = p->p_sib;
	}

	return p;
}
