/**
@file initial.c
@author
@note This module implements main() and exports the nucleus’s global variables. (e.g. Process Count, device semaphores, etc.)
 */
#include "../../phase1/h/asl.h"
#include "../../phase1/e/pcb.e"
#include "../../include/libuarm.h"
#include "../../include/const.h"
#include "../../include/arch.h"
#include "../../include/uARMconst.h"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../h/initial.h"
 
HIDDEN void populateArea(memaddr oldArea, memaddr handler)
{
	state_t *newArea;

	/* The new area points to the old area */
	newArea = (state_t *) oldArea;
	
	/* Save the current processor state */
	STST(newArea);
	
	/* Assign to Program Counter the Exception Handler address */
	newArea->pc = handler;
	newArea->sp = RAM_TOP;
	
	/* Masked interrupts; Virtual Memory off; Kernel Mode on */
	newArea->CP15_Control &= ~(0x1);
	newArea->cpsr = STATUS_SYS_MODE | STATUS_ALL_INT_DISABLE(newArea->cpsr);
}

void main(void)
{
	pcb_t *init;
	int i;

	/* Populate the processor state areas into the ROM Reserved Frame */
	populateArea(SYSBK_NEWAREA,		(memaddr) sysBpHandler);	/* SYS/BP Exception Handling */
	populateArea(PGMTRAP_NEWAREA,	(memaddr) pgmTrapHandler);	/* PgmTrap Exception Handling */
	populateArea(INT_NEWAREA,		(memaddr) intHandler);		/* Interrupt Exception Handling */
	populateArea(TLB_NEWAREA,		(memaddr) tlbHandler);		/* TLB Exception Handling */

	/* Initialize data structures */
	initPcbs();
	initASL();
	
	/* Initialize variables */
	readyQueue = mkEmptyProcQ();
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
		sem.disk[i] =
		sem.tape[i] =
		sem.network[i] =
		sem.printer[i] =
		sem.terminalR[i] =
		sem.terminalT[i] = 0;
	
	/* Initialize pseudo-clock semaphore */
	pseudo_clock = 0;
	
	/* Initialize init */
	if (!(init = allocPcb()))
		PANIC();
	
	/* Unmasked interrupts; Virtual Memory off; Kernel-Mode on */
	firstProc->p_s.CP15_Control &= ~(0x1);
	init->p_s.cpsr =  STATUS_SYS_MODE | STATUS_ALL_INT_ENABLE(init->p_s.cpsr);
	
	/* Initialize Stack Pointer */
	init->p_s.sp = RAM_TOP - BUS_REG_RAM_SIZE;
	
	/* Initialize Program Counter with the test process */
	init->p_s.pc = (memaddr) test;
	
	/* Initialize Process Id */
	init->p_pid = ++pidCount;
	
	/* Update PCB table */
	pcbused_table[0].pid = init->p_pid;
	pcbused_table[0].pcb = init;

	/* Insert init in ProcQ */
	insertProcQ(&readyQueue, init);
	processCount++;
	
	/* Start timer */
	startTimerTick = getTODLO();

	/* Run scheduler */
	scheduler();
}
