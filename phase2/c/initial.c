/**
@file initial.c
@author
@note This module implements main() and exports the nucleus’s global variables. (e.g. Process Count, device semaphores, etc.)
 */

#include "../h/initial.h"
 
HIDDEN void populate(memaddr oldArea, memaddr handler)
{
	state_t *newArea;

	/* The new area points to the old area */
	newArea = (state_t *) area;
	
	/* Save the current processor state area */
	STST(newArea);
	
	/* Assign to Program Counter the Exception Handler address */
	newArea->pc = newArea->ip = handler;
	newArea->sp = ROMF_EXCVTOP;
	
	/* Masked interrupts + Virtual Memory off + Kernel Mode on */
	newArea->cpsr = (newArea->cpsr | STATUS_KUc) & STATUS_ALL_INT_DISABLE & ~STATUS_VMp;
}

void main(void)
{
	pcb_t *init;
	int i;

	/* Populate the four processor state areas in the ROM Reserved Frame */
	populate(SYSBK_NEWAREA, (memaddr) sysBpHandler);		/* SYS/BP Exception Handling */
	populate(PGMTRAP_NEWAREA, (memaddr) pgmTrapHandler);	/* PgmTrap Exception Handling */
	populate(INT_NEWAREA, (memaddr) intHandler);			/* Interrupt Exception Handling */

	/* Initialize Level 2 data structures */
	initPcbs();
	initSemd();
	
	/* Initialize all nucleus maintained variables */
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
