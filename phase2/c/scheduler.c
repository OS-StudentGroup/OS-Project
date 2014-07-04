/**
@file scheduler.c
@author
@note Process scheduler and deadlock detection.
*/

#include "../h/scheduler.h"

void scheduler()
{
	/* [Case 1] A process is running */
	if (currentProcess)
	{		
		/* Set process start time in the CPU */
		currentProcess->p_cpu_time += (BUS_REG_TOD_LO - processTOD);
		processTOD = BUS_REG_TOD_LO;
		
		/* Update elapsed time of the pseudo-clock tick */
		timerTick += (BUS_REG_TOD_LO - startTimerTick);
		startTimerTick = BUS_REG_TOD_LO;
		
		/* Set Interval Timer as the smallest between timeslice and pseudo-clock tick */
		setTIMER(MIN((SCHED_TIME_SLICE - currentProcess->p_cpu_time), (SCHED_PSEUDO_CLOCK - timerTick)));

		/* Load the executing process state */
		LDST(&(currentProcess->p_state));
	}
	/* [Case 2] No process is running */
	else
	{
		/* If the Ready Queue is empty */
		if (emptyProcQ(&readyQueue))
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
				setSTATUS(STATUS_ALL_INT_ENABLE(STATUS_ENABLE_INT(getSTATUS())));
				while (TRUE);
			}
			PANIC(); /* Anomaly */
		}
		
		/* Extract first ready process */
		if (!(currentProcess = removeProcQ(&readyQueue)))
			PANIC(); /* Anomaly */
		
		/* Compute elapsed time from the pseudo-clock tick start */
		timerTick += BUS_REG_TOD_LO - startTimerTick;
		startTimerTick = BUS_REG_TOD_LO;
		
		/* Set process start time in the CPU */
		currentProcess->p_cpu_time = 0;
		processTOD = BUS_REG_TOD_LO;
		
		/* Set Interval Timer as the smallest between timeslice and pseudo-clock tick */
		setTIMER(MIN(SCHED_TIME_SLICE, (SCHED_PSEUDO_CLOCK - timerTick)));
		
		/* Load the processor state for execution */
		LDST(&(currentProcess->p_state));
	}
}
