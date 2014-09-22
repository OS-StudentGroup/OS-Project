/**
@file interrupts.c
@author
@note Interrupt Exception Handling.
*/

#include "../../phase1/h/asl.h"
#include "../../phase1/e/pcb.e"
#include "../e/exceptions.e"
#include "../h/initial.h"
#include "../e/interrupts.e"
#include "../e/scheduler.e"
#include "../../include/libuarm.h"
#include "../../include/const.h"
#include "../../include/arch.h"

/* Interrupt Old Area */
HIDDEN state_t *intOldArea = (state_t *) INT_OLDAREA;

/* Device Base Address */
HIDDEN memaddr deviceBaseAddress;

HIDDEN int getDevice(int bitmap)
{
	switch (bitmap)
	{
	case (bitmap | DEV_CHECK_ADDR_0): 	return IL_IPI;
	case (bitmap | DEV_CHECK_ADDR_1): 	return IL_CPUTIMER;
	case (bitmap | DEV_CHECK_ADDR_2): 	return IL_TIMER;
	case (bitmap | DEV_CHECK_ADDR_3): 	return IL_DISK;
	case (bitmap | DEV_CHECK_ADDR_4): 	return IL_TAPE;
	case (bitmap | DEV_CHECK_ADDR_5): 	return IL_ETHERNET;
	case (bitmap | DEV_CHECK_ADDR_6): 	return IL_PRINTER;
	default:							return IL_TERMINAL;
	}
}

HIDDEN pcb_t *verhogenInt(int *semaddr, int status, int line, int dev)
{
	pcb_t *process;
	
	(*semaddr)++;

	/* [Case 1] No blocked process */
	if (!(process = removeBlocked(semaddr)))
		statusWordDev[line][dev] = status;
	/* [Case 2] At least one blocked process */
	else
	{
		insertProcQ(&readyQueue, process);
		process->p_isOnDev = FALSE;
		softBlockCount--;
		process->p_s.v1 = status;
		p->p_s.a1 = status;
	}
	
	return process;
}

HIDDEN void intTimer()
{
	pcb_t *process;

	/* Update elapsed time */
	timerTick += getTODLO() - startTimerTick;
	startTimerTick = getTODLO();

	/* Check pseudo-clock tick */
	if (timerTick >= SCHED_PSEUDO_CLOCK)
	{
		/* [Case 1] At least one blocked processes */
		if (pseudo_clock < 0)
		{
			/* Unblock all processes (i.e. undo all previous SYS7 side effects) */
			for (; pseudo_clock < 0; pseudo_clock++)
			{
				/* If there is a blocked process */
				if ((process = removeBlocked(&pseudo_clock)))
				{
					/* Add the process into the Ready Queue */
					insertProcQ(&readyQueue, process);
					process->p_isOnDev = FALSE;
					process->p_isOnSem = FALSE;
					softBlockCount--;
				}
			}
		}
		/* [Case 2] At most one blocked process */
		else
		{
			/* [Case 2.1] 0 blocked process */
			if (!(process = removeBlocked(&pseudo_clock)))
				pseudo_clock--;
			/* [Case 2.2] 1 blocked process */
			else
			{
				/* Perform a V on the pseudo-clock */
				insertProcQ(&readyQueue, process);
				process->p_isOnDev = FALSE;
				process->p_isOnSem = FALSE;
				softBlockCount--;
				pseudo_clock++;
			}
		}

		/* Reset the timer tick used to compute the pseudo-clock tick */
		timerTick = 0;
		startTimerTick = getTODLO();
	}
	/* If time slice for current process ran out */
	else if (currentProcess)
	{
		/* Add current process into Ready Queue */
		insertProcQ(&readyQueue, currentProcess);
		currentProcess = NULL;
		softBlockCount++;

		/* Update elapsed time */
		timerTick += (getTODLO() - startTimerTick);
		startTimerTick = getTODLO();
	}
	/* Something else */
	else
		setTIMER(SCHED_PSEUDO_CLOCK - timerTick);
}

HIDDEN void devInterrupt(int cause)
{
	int *bitmap, deviceNumber;
	pcb_t *process;
	dtpreg_t *status;

	/* Get interrupting device bitmap starting address */
	bitmap = (int *) CDEV_BITMAP_ADDR(cause);

	/* Get highest priority device with pending interrupt */
	deviceNumber = getDevice(*bitmap);

	/* Get device status */
	status = (dtpreg_t*) DEV_REG_ADDR(cause, deviceNumber);

	/* Perform a V on the device semaphore */
	verhogenInt(&sem4.disk[deviceNumber], status->status);

	switch (cause)
	{
		case (INT_DISK):	process = verhogenInt(&sem.disk[devNumb], status->status);
							break;
		case (INT_TAPE):	process = verhogenInt(&sem.tape[devNumb], status->status);
							break;
		case (INT_UNUSED):	process = verhogenInt(&sem.network[devNumb], status->status);
							break;
		case (INT_PRINTER):	process = verhogenInt(&sem.printer[devNumb], status->status);
							break;
	}

	/* Ack on device to identify the pending interrupt */
	status->command = DEV_C_ACK;
}

HIDDEN void intTerminal()
{
	int *bitmap, deviceNumber;
	pcb_t *process;
	termreg_t *statusT;

	/* Get interrupting device bitmap starting address */
	bitmap = (int *) CDEV_BITMAP_ADDR(INT_TERMINAL);

	/* Get highest priority device with pending interrupt */
	deviceNumber = getDevice(*bitmap);

	/* Device status field */
	statusT = (termreg_t*) DEV_REG_ADDR(INT_TERMINAL, dNum);

	/* [Case 1] Sent character */
	if ((statusT->recv_status & DEV_TERM_STATUS) == DEV_TRCV_S_CHARRECV)
	{
		/* Perform a V on the device semaphore */
		verhogenInt(&sem4.terminalR[deviceNumber], statusT->recv_status);
		/* Ack on device to identify the pending interrupt */
		statusT->recv_command = DEV_C_ACK;
	}
	/* [Case 2] Received character */
	else if ((statusT->transm_status & DEV_TERM_STATUS) == DEV_TTRS_S_CHARTRSM)
	{
		/* Perform a V on the device semaphore */
		verhogenInt(&sem4.terminalT[dNum], statusT->transm_status);
		/* Ack on device to identify the pending interrupt */
		statusT->transm_command = DEV_C_ACK;
	}
}

void intHandler()
{
	int intCause;
	
	/* If a process is running,  */
	if (currentProcess)
	{
		/* Save current process state */
		saveCurrentState(intOldArea , &(currentProcess->p_s));

		/* Decrease Program Counter */
		currentProcess->p_s.pc -= 2 * WORD_SIZE;
	}

	/* Get interrupt cause */
	intCause = intOldArea->CP15_Cause;
	if 		(CAUSE_IP_GET(intCause, INT_TIMER))		intTimer();
	else if (CAUSE_IP_GET(intCause, INT_DISK))		devInterrupt(INT_DISK);
	else if (CAUSE_IP_GET(intCause, INT_TAPE))		devInterrupt(INT_TAPE);
	else if (CAUSE_IP_GET(intCause, INT_UNUSED))	devInterrupt(INT_UNUSED);
	else if (CAUSE_IP_GET(intCause, INT_PRINTER))	devInterrupt(INT_PRINTER);
	else if (CAUSE_IP_GET(intCause, INT_TERMINAL))	intTerminal();

	/* Run scheduler */
	scheduler();
}
