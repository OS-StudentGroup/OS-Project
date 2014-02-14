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

	// Pre-conditions: ProcQ is not empty and p is not NULL
	if (emptyProcQ(*tp) || !p) return NULL;

	// [Case 1] p is in the tail
	if (*tp == p)
	{
		// [Case 1.1] ProcQ has 1 ProcBlk
		if (hasOneProcBlk(*tp))
		{
			*tp = mkEmptyProcQ();
		}
		// [Case 1.2] ProcQ has >1 ProcBlk
		else
		{
			// Iterate PCB queue until the end is reached
			for (	it = (*tp)->p_next;
					it->p_next != *tp;
					it = it->p_next);

			it->p_next = it->p_next->p_next;
			*tp = it;
		}

		output = p;
		output->p_next = NULL;
	}
	// [Case 2] p is not in the tail
	else
	{
		// Iterate PCB queue until the end is reached OR p is found
		for (	it = *tp;
				it->p_next->p_next != *tp && it->p_next != p;
				it = it->p_next);

		// [Case 2.1] p is in ProcQ
		if (it->p_next == p)
		{
			it->p_next = it->p_next->p_next;

			output = p;
			output->p_next = NULL;
		}
		// [Case 2.2] p is not in ProcQ
		else
		{
			output = NULL;
		}
	}

	return output;
}
