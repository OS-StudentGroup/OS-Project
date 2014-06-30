/**
@file initial.c
@author
@note This module implements main() and exports the nucleus’s global variables. (e.g. Process Count, device semaphores, etc.)
 */

#include "../h/initial.h"
 
HIDDEN void populate(memaddr area, memaddr handler)
{
	state_t *newArea;
	
	newArea = (state_t *) area;
	STST(newArea);
	
	/* Imposta il PC all'indirizzo del gestore delle eccezioni */
	newArea->pc_epc = newArea->reg_t9 = handler;
	newArea->reg_sp = RAMTOP;
	
	/* Interrupt mascherati, Memoria Virtuale spenta, Kernel Mode attivo */
	newArea->status = (newArea->status | STATUS_KUc) & ~STATUS_INT_UNMASKED & ~STATUS_VMp;
}

void main(void)
{
	pcb_t *init;
	int i;

	/* Populate the four New Areas in the ROM Reserved Frame */
	populate(SYSBK_NEWAREA, (memaddr) sysBpHandler);		/* SYS/BP Exception Handling */
	populate(PGMTRAP_NEWAREA, (memaddr) pgmTrapHandler);	/* PgmTrap Exception Handling */
	populate(TLB_NEWAREA, (memaddr) tlbHandler);			/* TLB Exception Handling */
	populate(INT_NEWAREA, (memaddr) intHandler);			/* Interrupt Exception Handling */

	/* Initialize the Level 2 data structures */
	initPcbs();
	initSemd();
	
	/* Initialize all nucleus maintained variables */
	mkEmptyProcQ(&readyQueue);
	currentProcess = NULL;
	processCount = softBlockCount = pidCount = 0;
	timerTick = 0;
	
	/* Initialize PCB table */
	for (i = 0; i < MAXPROC; i++)
	{
		pcbused_table[i].pid = 0;
		pcbused_table[i].pcb = NULL;
	}
	
	/* Initialize device semaphores */
	for (i = 0; i < DEV_PER_INT; i++)
	{
		sem.disk[i] = 0;
		sem.tape[i] = 0;
		sem.network[i] = 0;
		sem.printer[i] = 0;
		sem.terminalR[i] = 0;
		sem.terminalT[i] = 0;
	}
	
	/* Initialize pseudo-clock semaphore */
	pseudo_clock = 0;
	
	/* Initialize init */
	if (!(init = allocPcb()))
		PANIC();
	
	/* Unmasked interrupts on; Virtual Memory off; Kernel-Mode on */
	init->p_state.status = (init->p_state.status | STATUS_IEp | STATUS_INT_UNMASKED | STATUS_KUc) & ~STATUS_VMp;
	
	/* Initialize SP */
	init->p_state.reg_sp = RAMTOP - FRAME_SIZE;
	
	/* Initialize PC */
	init->p_state.pc_epc = init->p_state.reg_t9 = (memaddr)test;
	
	/* Initialize PID */
	pidCount++;
	init->p_pid = pidCount;
	
	/* Update PCB table */
	pcbused_table[0].pid = init->p_pid;
	pcbused_table[0].pcb = init;

	/* Insert init in ProcQ */
	insertProcQ(&readyQueue, init);
	processCount++;
	
	/* Start timer */
	startTimerTick = GET_TODLOW;
	
	/* Run scheduler */
	scheduler();
}
