/**
@file scheduler.c
@author
@note Process scheduler and deadlock detection.
*/

#include "../e/scheduler.e"
#include "../h/initial.h"
#include "../../phase1/e/pcb.e"

void scheduler()
{
	/* tprint("Scheduler is running...\n"); */
	/* [Case 1] A process is running */
	if (currentProcess)
	{
		/* tprint("A process is running...\n"); */
		/* Set process start time in the CPU */
		currentProcess->p_cpu_time += (getTODLO() - processTOD);
		processTOD = getTODLO();
		
		/* Update elapsed time of the pseudo-clock tick */
		timerTick += (getTODLO() - startTimerTick);
		startTimerTick = getTODLO();
		
		/* Set Interval Timer as the smallest between timeslice and pseudo-clock tick */
		setTIMER(MIN((SCHED_TIME_SLICE - currentProcess->p_cpu_time), (SCHED_PSEUDO_CLOCK - timerTick)));

		/* Load the executing process state */
		LDST(&(currentProcess->p_s));
	}
	/* [Case 2] No process is running */
	else
	{
		/* tprint("Ready Queue is empty...\n"); */
		/* If the Ready Queue is empty */
		if (emptyProcQ(readyQueue))
		{
			/* No more processes */
			if (processCount == 0)
				HALT();
			/* Deadlock */
			if (processCount > 0 && softBlockCount == 0)
				PANIC();
			/* Wait state */
			if (processCount > 0 && softBlockCount > 0)
			{
				/* Unmasked interrupts on */
				setSTATUS(STATUS_ALL_INT_ENABLE(getSTATUS()));
				WAIT();
			}
			PANIC(); /* Anomaly */
		}
		
		/* Extract first ready process */
		if (!(currentProcess = removeProcQ(&readyQueue)))
			PANIC(); /* Anomaly */
		
		/* Compute elapsed time from the pseudo-clock tick start */
		timerTick += getTODLO() - startTimerTick;
		startTimerTick = getTODLO();
		
		/* Set process start time in the CPU */
		currentProcess->p_cpu_time = 0;
		processTOD = getTODLO();
		
		/* Set Interval Timer as the smallest between timeslice and pseudo-clock tick */
		setTIMER(MIN(SCHED_TIME_SLICE, (SCHED_PSEUDO_CLOCK - timerTick)));

		/* Load the processor state for execution */
		LDST(&(currentProcess->p_s));
	}
}
