/*
 * PCB handling functions
*/
#ifndef PCB_H
#define PCB_H

// Preprocessing directives
#include "../h/const.h"
#include "../h/types.h"

// Function declarations
// [List view functions]
EXTERN void freePcb(pcb_t *p);
EXTERN pcb_t *allocPcb(void);
EXTERN void initPcbs(void);
EXTERN pcb_t *mkEmptyProcQ(void);
EXTERN int emptyProcQ(pcb_t *tp);
EXTERN void insertProcQ(pcb_t **tp, pcb_t *p);
EXTERN pcb_t *removeProcQ(pcb_t **tp);
EXTERN pcb_t *outProcQ(pcb_t **tp, pcb_t *p);
EXTERN pcb_t *headProcQ(pcb_t *tp);
// [Tree view functions[
EXTERN int emptyChild(pcb_t *p);
EXTERN void insertChild(pcb_t *prnt, pcb_t *p);
EXTERN pcb_t *removeChild(pcb_t *p);
EXTERN pcb_t *outChild(pcb_t *p);

// Private global variable that points to the head of the pcbFree list
HIDDEN pcb_t *pcbFree_h;

/*
 * Insert the element pointed to by p onto the pcbFree list.
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
*/
EXTERN pcb_t *allocPcb(void)
{
	pcb_t *pcb, *output;

	pcb = removeProcQ(&pcbFree_h);
	// [Case 1] pcbFree is not empty
	if(pcb)
	{
		pcb->p_next = NULL;
		pcb->p_prev = NULL;
		pcb->p_prnt = NULL;
		pcb->p_child = NULL;
		pcb->p_sib = NULL;
		pcb->p_prev_sib = NULL;
		output = pcb;
	}
	// [Case 2] pcbFree is empty
	else
	{
		output = NULL;
	}
	return output;
}

/*
 * Initialize the pcbFree list to contain all the elements of the
 * static array of MAXPROC ProcBlk’s.
 * This method will be called only once during data structure initialization.
*/
EXTERN void initPcbs(void)
{
	HIDDEN pcb_t pcbs[MAXPROC];
	int i;

	pcbFree_h = mkEmptyProcQ();
	for(i = 0; i < MAXPROC; i++)
	{
		insertProcQ(&pcbFree_h, &pcbs[i]);
	}
}

/*
 * This method is used to initialize a variable to be tail pointer to a
 * process queue.
 * Return a pointer to the tail of an empty process queue;
 * i.e. NULL.
*/
EXTERN pcb_t *mkEmptyProcQ(void)
{
	return NULL;
}

/*
 * Return TRUE if the queue whose tail is pointed to by tp is empty.
 * Return FALSE otherwise.
*/
EXTERN int emptyProcQ(pcb_t *tp)
{
	return tp? FALSE : TRUE;
}

/*
 * Insert the ProcBlk pointed to by p into the process queue whose
 * tail-pointer is pointed to by tp.
 * Note the double indirection through tp to allow for the possible
 * updating of the tail pointer as well.
*/
EXTERN void insertProcQ(pcb_t **tp, pcb_t *p)
{
	// [Case 1] ProcQ is empty
	if(emptyProcQ((*tp)))
	{
		(*tp) = p;
		(*tp)->p_next = (*tp);
		(*tp)->p_prev = (*tp);
	}
	// [Case 2] ProcQ is not empty
	else
	{
		p->p_next = (*tp)->p_next;
		(*tp)->p_next->p_prev = p;
		(*tp)->p_next = p;
		p->p_prev = (*tp);
		(*tp) = p;
	}
}

/*
 * Remove the first (i.e. head) element from the process queue whose
 * tail-pointer is pointed to by tp.
 * Return NULL if the process queue was initially empty; otherwise
 * return the pointer to the removed element.
 * Update the process queue’s tail pointer if necessary.
*/
EXTERN pcb_t *removeProcQ(pcb_t **tp)
{
	pcb_t *output;
	// [Case 1] ProcQ is empty
	if(emptyProcQ(*tp))
	{
		output = NULL;
	}
	// [Case 2] ProcQ is not empty
	else
	{
		// [Case 2.1] ProcQ has 1 ProcBlk
		if((*tp)->p_next == *tp)
		{
			output = (*tp);
			*tp = mkEmptyProcQ();
		}
		// [Case 2.2] ProcQ has >1 ProcBlk
		else
		{
			output = (*tp)->p_next;
			(*tp)->p_next->p_next->p_prev = *tp;
			(*tp)->p_next = (*tp)->p_next->p_next;
		}
	}
	return output;
}

/*
 * Remove the ProcBlk pointed to by p from the process queue
 * whose tail-pointer is pointed to by tp.
 * Update the process queue’s tail pointer if necessary.
 * If the desired entry is not in the indicated queue
 * (an error condition), return NULL; otherwise, return p.
 * Note that p can point to any element of the process queue.
*/
EXTERN pcb_t *outProcQ(pcb_t **tp, pcb_t *p)
{
	pcb_t *output, *it;

	// [Case 1] ProcQ is empty
	if(emptyProcQ(*tp))
	{
		output = NULL;
	}
	// [Case 2] ProcQ is not empty
	else
	{
		// [Case 2.1] ProcQ has 1 ProcBlk
		if((*tp)->p_next == *tp)
		{
			// [Case 2.1.1] p is in the process queue
			if(*tp == p)
			{
				output = *tp;
				*tp = mkEmptyProcQ();
			}
			// [Case 2.1.2] p is not in the process queue
			else
			{
				output = NULL;
			}
		}
		// [Case 2.2] ProcQ has >1 ProcBlk
		else
		{
			it = tp;
			// Iterate PCB queue until
			do
			{
				it = it->p_next;
			}
			// the end is reached OR the ProcBlk is found
			while(it != tp && it != p);
			// [Case 2.2.1] p is in the process queue
			if(p == it)
			{
				it->p_prev->p_next = it->p_next;
				it->p_next->p_prev = it->p_prev;
				// If p is in the tail
				if(it == tp)
				{
					*tp = (*tp)->p_prev;
				}
				output = it;
			}
			// [Case 2.2.2] p is not in the process queue
			else
			{
				output = NULL;
			}
		}
	}
	return output;
}

/*
 * Return a pointer to the first ProcBlk from the process queue whose
 * tail is pointed to by tp.
 * Do not remove this ProcBlk from the process queue.
 * Return NULL if the process queue is empty.
*/
EXTERN pcb_t *headProcQ(pcb_t *tp)
{
	return emptyProcQ(tp)? NULL : (tp)->p_next;
}

/*
 * Check if a ProcBlk has no children
 * Input:	pcb_t *p,	pointer to a ProcBlk
 * Output:	int
 * 						TRUE, 	if ProcBlk has no children
 * 						FALSE, 	otherwise
*/
EXTERN int emptyChild(pcb_t *p)
{
	int output;
	// [Case 1] ProcBlk is not NULL
	if(p)
	{
		// [Case 1.1] ProcBlk has children
		if(p->p_child)
		{
			output = FALSE;
		}
		// [Case 1.2] ProcBlk has no children
		else
		{
			output = TRUE;
		}
	}
	// [Case 2] ProcBlk is NULL
	else
	{
		output = TRUE;
	}
	return output;
}

/*
 * Make the ProcBlk pointed to by p a child of the ProcBlk
 * pointed to by prnt.
*/
EXTERN void insertChild(pcb_t *prnt, pcb_t *p)
{
	pcb_t *it;
	// Preconditions
	if(!prnt || !p) return;

	p->p_sib = NULL;
	p->p_prnt = prnt;
	// [Case 1] p is the first child
	if(emptyChild(prnt))
	{
		prnt->p_child = p;
	}
	// [Case 2] p is not the first child
	else
	{
		// Reach last sibling
		for(it = prnt->p_child; it->p_sib; it = it->p_sib);
		it->p_sib = p;
	}
}

/*
 * Make the first child of the ProcBlk pointed to by p no
 * longer a child of p.
 * Return NULL if initially there were no children of p.
 * Otherwise, return a pointer to this removed first child ProcBlk.
*/
EXTERN pcb_t *removeChild(pcb_t *p)
{
	pcb_t *output;

	// [Case 1] ProcBlk has no children
	if(emptyChild(p))
	{
		output = NULL;
	}
	// [Case 2] ProcBlk has children
	else
	{
		output = p->p_child;
		p->p_child = p->p_child->p_sib;
	}
	return output;
}

/*
 * Make the ProcBlk pointed to by p no longer the child
 * of its parent.
 * If the ProcBlk pointed to by p has no parent, return NULL;
 * otherwise, return p.
 * Note that the element pointed to by p need not be the first
 * child of its parent.
*/
EXTERN pcb_t *outChild(pcb_t *p)
{
	pcb_t *output, *it;
	// [Case 1] ProcBlk has no parent
	if(!p->p_prnt)
	{
		output = NULL;
	}
	// [Case 2] ProcBlk has parent
	else
	{
		p->p_prnt = NULL;
		p->p_sib = NULL;
		// [Case 2.1] ProcBlk is the first child
		if(p->p_prnt->p_child == p)
		{
			p->p_prnt->p_child = p->p_sib;
		}
		// [Case 2.2] ProcBlk is not the first child
		else
		{
			// Iterate the children
			for(it = p->p_prnt->p_child;
				it->p_sib != p;
				it = it->p_sib);
			it->p_sib = p->p_sib;
		}
		output = p;
	}
	return output;
}
#endif
