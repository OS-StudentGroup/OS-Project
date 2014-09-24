/**
@file initial.c
@note This module implements main() and exports the nucleus’s global variables. (e.g. Process Count, device semaphores, etc.)
 */

#include "../e/inclusions.e"

EXTERN void test();

/* Global variables declarations */
EXTERN pcb_t *ReadyQueue;										/**< Process ready queue */
EXTERN pcb_t *CurrentProcess;									/**< Pointer to the executing PCB */
EXTERN U32 ProcessCount;										/**< Process counter */
EXTERN U32 PidCount;											/**< PID counter */
EXTERN U32 SoftBlockCount;										/**< Blocked process counter */
EXTERN int StatusWordDev[STATUS_WORD_ROWS][STATUS_WORD_COLS];	/**< Device Status Word table */
EXTERN SemaphoreStruct Semaphore;								/**< Device semaphores per line */
EXTERN int PseudoClock;											/**< Pseudo-clock semaphore */
EXTERN cpu_t ProcessTOD;										/**< Process start time */
EXTERN int TimerTick;											/**< Tick timer */
EXTERN cpu_t StartTimerTick;									/**< Pseudo-clock tick start time */

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
	newArea->cpsr = STATUS_SYS_MODE | STATUS_ALL_INT_DISABLE(newArea->cpsr);
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
	ProcessCount = SoftBlockCount = PidCount = TimerTick = 0;
	
	/* Initialize device semaphores */
	for (i = 0; i < DEV_PER_INT; i++)
		Semaphore.disk[i] =
		Semaphore.tape[i] =
		Semaphore.network[i] =
		Semaphore.printer[i] =
		Semaphore.terminalR[i] =
		Semaphore.terminalT[i] = 0;
	
	/* Initialize Pseudo-Clock semaphore */
	PseudoClock = 0;
	
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
	
	/* Initialize Process Id */
	init->p_pid = ++PidCount;

	/* Insert init in ProcQ */
	insertProcQ(&ReadyQueue, init);
	ProcessCount++;
	
	/* Start the timer tick */
	StartTimerTick = getTODLO();

	/* Call the scheduler */
	scheduler();

	/* Anomaly */
	PANIC();
	return -1;
}
