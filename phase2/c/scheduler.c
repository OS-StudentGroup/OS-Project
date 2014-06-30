/**
 *  @file scheduler.c
 *  @author
 *  @note Process scheduler and deadlock detection.
 */

#include "../h/scheduler.h"

void scheduler()
{
	/* [Case 1] A process is running */
	if (currentProcess)
	{		
		/* Set process start time in the CPU */
		currentProcess->p_cpu_time += (GET_TODLOW - processTOD);
		processTOD = GET_TODLOW;
		
		/* Update elapsed time of the pseudo-clock tick*/
		timerTick += (GET_TODLOW - startTimerTick);
		startTimerTick = GET_TODLOW;
		
		/* Set Interval Timer as the smallest between the timeslice and the pseudo-clock tick */
		SET_IT(MIN((SCHED_TIME_SLICE - currentProcess->p_cpu_time), (SCHED_PSEUDO_CLOCK - timerTick)));

		/* Load the executing process state */
		LDST(&(currentProcess->p_state));
	}
	/* [Case 2] No process is running */
	else
	{
		/* If the Ready Queue is empty */
		if (emptyProcQ(&readyQueue))
		{
			if (processCount == 0)
				HALT();
			/* Deadlock */
			if ((processCount > 0) && (softBlockCount == 0))
				PANIC();
			/* Wait State */
			if ((processCount > 0) && (softBlockCount > 0))
			{
				/* Unmasked interrupts on */
				setSTATUS((getSTATUS() | STATUS_IEc | STATUS_INT_UNMASKED));
				while (TRUE) ;
			}
			/* Anomaly */
			PANIC();
		}
		
		/* Extract first ready process */
		currentProcess = removeProcQ(&readyQueue);
		
		/* Anomaly */
		if (!currentProcess)
			PANIC();
		
		/* Compute elapsed time from the pseudo-clock tick start */
		timerTick += GET_TODLOW - startTimerTick;
		startTimerTick = GET_TODLOW;
		
		/* Set process start time in the CPU */
		currentProcess->p_cpu_time = 0;
		processTOD = GET_TODLOW;
		
		/* Set Interval Timer as the smallest between the timeslice and the pseudo-clock tick */
		SET_IT(MIN(SCHED_TIME_SLICE, (SCHED_PSEUDO_CLOCK - timerTick)));
		
		/* Load the executing process state */
		LDST(&(currentProcess->p_state));
	}
}
