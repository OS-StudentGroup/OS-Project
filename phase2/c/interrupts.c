/**
@file interrupts.c
@note Interrupt Exception Handling.
*/

#include "../e/inclusions.e"
#include "../../include/types.h"

/* Interrupt Old Area */
HIDDEN state_t *InterruptOldArea = (state_t *) INT_OLDAREA;

/* Device Check Addresses */
HIDDEN const int DeviceCheckAddress[8] =
{
		DEV_CHECK_ADDRESS_0,
		DEV_CHECK_ADDRESS_1,
		DEV_CHECK_ADDRESS_2,
		DEV_CHECK_ADDRESS_3,
		DEV_CHECK_ADDRESS_4,
		DEV_CHECK_ADDRESS_5,
		DEV_CHECK_ADDRESS_6,
		DEV_CHECK_ADDRESS_7
 };

/* Device Check Lines*/
HIDDEN const int InterruptLine[8] =
{
		IL_IPI,
		IL_CPUTIMER,
		IL_TIMER,
		IL_DISK,
		IL_TAPE,
		IL_ETHERNET,
		IL_PRINTER,
		IL_TERMINAL
};

/*
@brief Identify at which device a pending interrupt refers to.
@param bitmap Bitmap of the pending interrupt.
@return The device identifier.
*/
HIDDEN int getDevice(int bitmap)
{
	/*int i;
	for (i = 0; bitmap != (bitmap | DeviceCheckAddress[i]) && i < 7; i++);
	return InterruptLine[i];*/


	if (bitmap == (bitmap | DEV_CHECK_ADDRESS_0)) return IL_IPI;
	else if (bitmap == (bitmap | DEV_CHECK_ADDRESS_1)) return IL_CPUTIMER;
	else if (bitmap == (bitmap | DEV_CHECK_ADDRESS_2)) return IL_TIMER;
	else if (bitmap == (bitmap | DEV_CHECK_ADDRESS_3)) return IL_DISK;
	else if (bitmap == (bitmap | DEV_CHECK_ADDRESS_4)) return IL_TAPE;
	else if (bitmap == (bitmap | DEV_CHECK_ADDRESS_5)) return IL_ETHERNET;
	else if (bitmap == (bitmap | DEV_CHECK_ADDRESS_6)) return IL_PRINTER;
	else return IL_TERMINAL;
}

/*
@brief Performs a V on the given device semaphore.
@param semaddr Address of the semaphore.
@param status Device status.
@return Void.
*/
HIDDEN void verhogenInt(int *semaddr, int status)
{
	pcb_t *process;
	
	/* Performs a V on the semaphore */
	(*semaddr)++;

	/* If there is at least one blocked process */
	if (!(process = removeBlocked(semaddr)))
	{
		/* Add the process into the Ready Queue */
		insertProcQ(&ReadyQueue, process);

		SoftBlockCount--;
		process->p_s.a1 = status;
		/* process->p_isBlocked = FALSE;*/
	}
}

/*
@brief Acknowledge a pending interrupt on the Timer Click.
@return Void.
*/
HIDDEN void intTimer()
{
	pcb_t *process;

	/* Update elapsed time */
	TimerTick += getTODLO() - StartTimerTick;
	StartTimerTick = getTODLO();

	/* [Case 1] The Time Slice for the current process did not ran out */
	if (TimerTick >= SCHED_PSEUDO_CLOCK)
	{
		/* [Case 1.1] There is at least one blocked process */
		if (PseudoClock < 0)
		{
			/* Unblock all processes */
			while (PseudoClock < 0)
			{
				/* If there is a blocked process */
				if ((process = removeBlocked(&PseudoClock)))
				{
					/* Add the process into the Ready Queue */
					insertProcQ(&ReadyQueue, process);

					process->p_isBlocked = FALSE;
					SoftBlockCount--;
				}
				PseudoClock++;
			}
		}
		/* [Case 1.2] There is at most one blocked process */
		else
		{
			/* [Case 1.2.1] 0 blocked processes */
			if (!(process = removeBlocked(&PseudoClock)))
				/* Perform a P on the Pseudo-Clock */
				PseudoClock--;
			/* [Case 1.2.2] 1 blocked process */
			else
			{
				/* Add the process into the Ready Queue */
				insertProcQ(&ReadyQueue, process);

				process->p_isBlocked = FALSE;
				SoftBlockCount--;

				/* Perform a V on the Pseudo-Clock */
				PseudoClock++;
			}
		}

		/* Reset the timer tick used to compute the Pseudo-Clock tick */
		TimerTick = 0;
		StartTimerTick = getTODLO();
	}
	/* [Case 2] The Time Slice for the current process ran out */
	else if (CurrentProcess)
	{
		/* Add the current process into the Ready Queue */
		insertProcQ(&ReadyQueue, CurrentProcess);
		CurrentProcess = NULL;
		SoftBlockCount++;

		/* Update elapsed time */
		TimerTick += getTODLO() - StartTimerTick;
		StartTimerTick = getTODLO();
	}
	/* [Case 3] There is not a running process */
	else
		setTIMER(SCHED_PSEUDO_CLOCK - TimerTick);
}

/*
@brief Acknowledge a pending interrupt through setting the command code in the device register.
@return Void.
*/
HIDDEN void devInterrupt(int cause)
{
	int *deviceBitmap, deviceNumber;
	dtpreg_t *status;

	/* Get the starting address of the device bitmap */
	deviceBitmap = (int *) CDEV_BITMAP_ADDR(cause);

	/* Get the highest priority device affected by a pending interrupt */
	deviceNumber = getDevice(*deviceBitmap);

	/* Get the device status */
	status = (dtpreg_t *) DEV_REG_ADDR(cause, deviceNumber);

	/* Perform a V on the device semaphore */
	switch (cause)
	{
		case (INT_DISK):	verhogenInt(&Semaphore.disk[deviceNumber], status->status);		break;
		case (INT_TAPE):	verhogenInt(&Semaphore.tape[deviceNumber], status->status);		break;
		case (INT_UNUSED):	verhogenInt(&Semaphore.network[deviceNumber], status->status);	break;
		case (INT_PRINTER):	verhogenInt(&Semaphore.printer[deviceNumber], status->status);	break;
	}

	/* Identify the pending interrupt */
	status->command = DEV_C_ACK;
}

/*
@brief Acknowledge a pending interrupt on the terminal, distinguishing between receiving and sending ones.
@return Void.
*/
HIDDEN void intTerminal()
{
	int *deviceBitmap, deviceNumber;
	termreg_t *status;

	/* Get the starting address of the device bitmap */
	deviceBitmap = (int *) CDEV_BITMAP_ADDR(INT_TERMINAL);

	/* Get the highest priority device affected by a pending interrupt */
	deviceNumber = getDevice(*deviceBitmap);

	/* Get the device status */
	status = (termreg_t *) DEV_REG_ADDR(INT_TERMINAL, deviceNumber);

	/* [Case 1] Sending a character */
	if ((status->recv_status & DEV_TERM_STATUS) == DEV_TRCV_S_CHARRECV)
	{
		/* Perform a V on the device semaphore */
		verhogenInt(&Semaphore.terminalR[deviceNumber], status->recv_status);

		/* Identify the pending interrupt */
		status->recv_command = DEV_C_ACK;
	}
	/* [Case 2] Receiving a character */
	else if ((status->transm_status & DEV_TERM_STATUS) == DEV_TTRS_S_CHARTRSM)
	{
		/* Perform a V on the device semaphore */
		verhogenInt(&Semaphore.terminalT[deviceNumber], status->transm_status);

		/* Identify the pending interrupt */
		status->transm_command = DEV_C_ACK;
	}
}


/*
@brief The function identifies the pending interrupt and performs a V on the related semaphore.
@return Void.
*/
EXTERN void intHandler()
{
	int interruptCause;
	
	/* If there is a running process */
	if (CurrentProcess)
	{
		/* Save current processor state */
		saveCurrentState(InterruptOldArea , &(CurrentProcess->p_s));

		/* Decrease Program Counter */
		CurrentProcess->p_s.pc -= 2 * WORD_SIZE;
	}

	/* Get the interrupt cause and call the handler accordingly */
	interruptCause = InterruptOldArea->CP15_Cause;
	if 		(CAUSE_IP_GET(interruptCause, INT_TIMER))		intTimer();					/* Timer */
	else if (CAUSE_IP_GET(interruptCause, INT_DISK))		devInterrupt(INT_DISK);		/* Disk */
	else if (CAUSE_IP_GET(interruptCause, INT_TAPE))		devInterrupt(INT_TAPE);		/* Tape */
	else if (CAUSE_IP_GET(interruptCause, INT_UNUSED))		devInterrupt(INT_UNUSED);	/* Unused */
	else if (CAUSE_IP_GET(interruptCause, INT_PRINTER))		devInterrupt(INT_PRINTER);	/* Printer */
	else if (CAUSE_IP_GET(interruptCause, INT_TERMINAL))	intTerminal();				/* Terminal */
	
	/* Call the scheduler */
	scheduler();
}
