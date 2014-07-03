/**
@file initial.c
@author
@note This module implements main() and exports the nucleus’s global variables. (e.g. Process Count, device semaphores, etc.)
 */

#include "../h/initial.h"
 
HIDDEN void populateArea(memaddr oldArea, memaddr handler)
{
	state_t *newArea;

	/* The new area points to the old area */
	newArea = (state_t *) oldArea;
	
	/* Save the current processor state area */
	STST(newArea);
	
	/* Assign to Program Counter the Exception Handler address */
	newArea->pc = newArea->ip = handler;
	newArea->sp = RAM_BASE;
	
	/* Masked interrupts; Virtual Memory off; Kernel Mode on */
	newArea->cpsr = newArea->cpsr | STATUS_SYS_MODE | STATUS_ALL_INT_DISABLE;
}

void main(void)
{
	pcb_t *init;
	int i;

	/* Populate the 4 processor state areas in the ROM Reserved Frame */
	populateArea(SYSBK_NEWAREA, (memaddr) sysBpHandler);		/* SYS/BP Exception Handling */
	populateArea(PGMTRAP_NEWAREA, (memaddr) pgmTrapHandler);	/* PgmTrap Exception Handling */
	populateArea(INT_NEWAREA, (memaddr) intHandler);			/* Interrupt Exception Handling */
	populateArea(TLB_NEWAREA, (memaddr) tlbHandler);			/* TLB Exception Handling */

	/* Initialize data structures */
	initPcbs();
	initSemd();
	
	/* Initialize nucleus maintained variables */
	mkEmptyProcQ(&readyQueue);
	currentProcess = NULL;
	processCount = softBlockCount = pidCount = timerTick = 0;
	
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
	
	/* Unmasked interrupts; Virtual Memory off; Kernel-Mode on */
	init->p_state.cpsr = init->p_state.cpsr | STATUS_SYS_MODE | STATUS_ALL_INT_ENABLE;
	
	/* Initialize Stack Pointer */
	init->p_state.sp = RAMTOP - FRAME_SIZE;
	
	/* Initialize Program Counter */
	init->p_state.pc = (memaddr) test;
	
	/* Initialize Process Id */
	pidCount++;
	init->p_pid = pidCount;
	
	/* Update PCB table */
	pcbused_table[0].pid = init->p_pid;
	pcbused_table[0].pcb = init;

	/* Insert init in ProcQ */
	insertProcQ(&readyQueue, init);
	processCount++;
	
	/* Start timer */
	startTimerTick = 0;
	
	/* Run scheduler */
	scheduler();
}
