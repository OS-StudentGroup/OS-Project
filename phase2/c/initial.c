/**
@file initial.c
@note This module implements main() and exports the nucleus’s global variables. (e.g. Process Count, device semaphores, etc.)
 */

#include "../e/inclusions.e"
#include "../../include/types.h"

EXTERN void test();

pcb_t *ReadyQueue;
pcb_t *CurrentProcess;
U32 ProcessCount;
U32 SoftBlockCount;
SemaphoreStruct Semaphore;
int PseudoClock;
U32 ProcessTOD;
int TimerTick;
U32 StartTimerTick;

/*
@brief Populate a new processor state area.
@param area Physical address of the area.
@param handler Physical address of the handler.
@return Void.
*/
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
	newArea->cpsr = STATUS_ALL_INT_DISABLE(newArea->cpsr) | STATUS_SYS_MODE;
}

int main()
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
	
	/* Initialize global variables */
	ReadyQueue = mkEmptyProcQ();
	CurrentProcess = NULL;
	ProcessCount = SoftBlockCount = TimerTick = PseudoClock = 0;
	
	/* Initialize device semaphores */
	for (i = 0; i < DEV_PER_INT; i++)
		Semaphore.disk[i] =
		Semaphore.tape[i] =
		Semaphore.network[i] =
		Semaphore.printer[i] =
		Semaphore.terminalR[i] =
		Semaphore.terminalT[i] = 0;

	/* Initialize init method */
	if (!(init = allocPcb()))
		PANIC(); /* Anomaly */
	
	/* Enable interrupts; enable Kernel-Mode; disable Virtual Memory */
	init->p_s.CP15_Control &= ~(0x1);
	init->p_s.cpsr =  STATUS_SYS_MODE | STATUS_ALL_INT_ENABLE(init->p_s.cpsr);
	
	/* Initialize Stack Pointer */
	init->p_s.sp = RAM_TOP - BUS_REG_RAM_SIZE;
	
	/* Initialize Program Counter with the test process */
	init->p_s.pc = (memaddr) test;

	/* Insert init in ProcQ */
	insertProcQ(&ReadyQueue, init);
	
	/* Initialize Process Id */
	ProcessCount++;

	/* Start the timer tick */
	StartTimerTick = getTODLO();
	/* Call the scheduler */
	scheduler();

	/* Anomaly */
	PANIC();
	return 0;
}
