/**
@file initial.c
@note This module implements main() and defines the nucleus’s global variables.
*/

#include "../e/dependencies.e"

EXTERN void test ();

/* Global variables declarations */
pcb_t *ReadyQueue;						/**< Process ready queue */
pcb_t *CurrentProcess;					/**< Pointer to the executing PCB */
U32 ProcessCount;						/**< Process counter */
U32 SoftBlockCount;						/**< Blocked process counter */
U32 ProcessTOD;							/**< Process start time */
U32 TimerTick;							/**< Tick timer */
U32 StartTimerTick;						/**< Pseudo-clock tick start time */
DeviceSemaphores Semaphores;			/**< Device semaphores per line */
int PseudoClock;						/**< Pseudo-clock semaphore */

/**
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

	/* Disable Virtual Memory */
	newArea->CP15_Control &= ~(0x1);

	/* Disable interrupts */
	newArea->cpsr = STATUS_ALL_INT_DISABLE(newArea->cpsr);

	/* Activate Kernel Mode */
	newArea->cpsr |= STATUS_SYS_MODE;
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
		Semaphores.disk[i] = Semaphores.tape[i] = Semaphores.network[i] =
		Semaphores.printer[i] = Semaphores.terminalR[i] = Semaphores.terminalT[i] = 0;

	/* Initialize init method */
	if (!(init = allocPcb())) PANIC(); /* Anomaly */

	/* Disable Virtual Memory */
	init->p_s.CP15_Control &= ~(0x1);

	/* Enable interrupts */
	init->p_s.cpsr = STATUS_ALL_INT_ENABLE(init->p_s.cpsr);

	/* Activate Kernel Mode */
	init->p_s.cpsr |= STATUS_SYS_MODE;

	/* Initialize Stack Pointer */
	init->p_s.sp = RAM_TOP - BUS_REG_RAM_SIZE;

	/* Initialize Program Counter with the test process */
	init->p_s.pc = (memaddr) test;

	/* Insert init in ProcQ */
	insertProcQ(&ReadyQueue, init);

	/* Increment Process Count */
	ProcessCount++;

	/* Start the timer tick */
	StartTimerTick = getTODLO();

	/* Call the scheduler */
	scheduler();

	/* Anomaly */
	PANIC();
	return 0;
}
