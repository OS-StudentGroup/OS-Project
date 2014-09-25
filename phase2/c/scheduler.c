/**
@file scheduler.c
@note Process scheduler and deadlock detection.
*/

#include "../e/dependencies.e"

/**
@brief The function updates the CPU time of the running process and re-start the Timer Tick.
In case there is not a running process, the function performs deadlock detection, initializes
the CPU time and starts the first process in the Ready Queue.
@return Void.
*/
void scheduler()
{
	/* [Case 1] There is a running process */
	if (CurrentProcess)
	{
		/* Set process start time in the CPU */
		CurrentProcess->p_cpu_time += getTODLO() - ProcessTOD ;
		ProcessTOD  = getTODLO();

		/* Update elapsed time of the Pseudo-Clock tick */
		TimerTick  += getTODLO() - StartTimerTick   ;
		StartTimerTick    = getTODLO();

		/* Set Interval Timer as the smallest between Time Slice and Pseudo-Clock tick */
		setTIMER(MIN((SCHED_TIME_SLICE - CurrentProcess->p_cpu_time), (SCHED_PSEUDO_CLOCK - TimerTick )));

		/* Load the processor state in order to start execution */
		LDST(&(CurrentProcess->p_s));
	}
	/* [Case 2] There is not a running process */
	else
	{
		/* If Ready Queue is empty */
		if (emptyProcQ(ReadyQueue ))
		{
			/* [Case 2.1] There are no more processes */
			if (ProcessCount  == 0)
				HALT();
			/* [Case 2.2] Deadlock Detection */
			if (ProcessCount  > 0 && SoftBlockCount  == 0)
				PANIC();
			/* [Case 2.3] At least one process is blocked */
			if (ProcessCount  > 0 && SoftBlockCount > 0)
			{
				/* Enable interrupts */
				setSTATUS(STATUS_ALL_INT_ENABLE(getSTATUS()));

				/* Set the machine in idle state waiting for interrupts */
				WAIT();
			}
			PANIC(); /* Anomaly */
		}

		/* Otherwise extract first ready process */
		if (!(CurrentProcess = removeProcQ(&ReadyQueue )))
			PANIC(); /* Anomaly */

		/* Compute elapsed time from the Pseudo-Clock tick */
		TimerTick  += getTODLO() - StartTimerTick   ;
		StartTimerTick    = getTODLO();

		/* Initialize CPU time */
		CurrentProcess->p_cpu_time = 0;
		ProcessTOD  = getTODLO();

		/* Set Interval Timer as the smallest between Time Slice and Pseudo-Clock tick */
		setTIMER(MIN(SCHED_TIME_SLICE, (SCHED_PSEUDO_CLOCK - TimerTick )));

		/* Load the processor state in order to start execution */
		LDST(&(CurrentProcess->p_s));
	}
}
