/**
@file interrupts.c
@note Interrupt Exception Handling.
*/

#include "../e/dependencies.e"
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

/* Device Check Lines */
HIDDEN const int InterruptLine[8] =
{
		DEV_CHECK_LINE_0,
		DEV_CHECK_LINE_1,	/* Local (CPU) Timer */
		DEV_CHECK_LINE_2,	/* Interval Timer */
		DEV_CHECK_LINE_3,	/* Disk  */
		DEV_CHECK_LINE_4,	/* Tape */
		DEV_CHECK_LINE_5,	/* Ethernet */
		DEV_CHECK_LINE_6,	/* Printer */
		DEV_CHECK_LINE_7	/* Terminal */
};

/**
 *	getDevice - recognize device with a pending interrupt on the bitmap passed
 *	@deviceMapAddr: bitmap of pending interrupt for interrupt line
 *
 *	returns: the index of the device with a pending interrupt
 */
HIDDEN int getDevice(int bitmap)
{
	int i;
	for (i = 0; bitmap != (bitmap | DeviceCheckAddress[i]) && i < 7; i++);
	return InterruptLine[i];
}

/*
@brief Performs a V on the given device Semaphores.
@param semaddr Address of the Semaphores.
@param status Device status.
@return Void.
*/
HIDDEN void verhogenInt(int *semaddr, int status)
{
	pcb_t *process;

	/* Performs a V on the semaphore */
	(*semaddr)++;

	/* If there is at least one blocked process */
	if ((process = removeBlocked(semaddr)))
	{
		/* Add the process into the Ready Queue */
		insertProcQ(&ReadyQueue, process);

		SoftBlockCount--;
		process->p_s.a1 = status;
		process->p_isBlocked = FALSE;
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
			for (; PseudoClock < 0; PseudoClock++)
			{
				/* If there is a blocked process */
				if ((process = removeBlocked(&PseudoClock)))
				{
					/* Add the process into the Ready Queue */
					insertProcQ(&ReadyQueue, process);
					process->p_isBlocked = FALSE;
					SoftBlockCount--;
				}
			}
		}
		/* [Case 1.2] There is at most one blocked process */
		else
		{
			/* [Case 1.2.1] 0 blocked processes */
			if (!(process = removeBlocked(&PseudoClock)))
			{
				/* Perform a P on the Pseudo-Clock */
				PseudoClock--;
			}
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
		CurrentProcess->p_isBlocked = TRUE;
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
	dtpreg_t *deviceRegister;

	/* Get the starting address of the device bitmap */
	deviceBitmap = (int *) CDEV_BITMAP_ADDR(cause);

	/* Get the highest priority device affected by a pending interrupt */
	deviceNumber = getDevice(*deviceBitmap);

	/* Get the device register */
	deviceRegister = (dtpreg_t *) DEV_REG_ADDR(cause, deviceNumber);

	/* Perform a V on the device semaphore */
	switch (cause)
	{
	case (INT_DISK):	verhogenInt(&Semaphores.disk[deviceNumber], 	deviceRegister->status);	break;
	case (INT_TAPE):	verhogenInt(&Semaphores.tape[deviceNumber], 	deviceRegister->status);	break;
	case (INT_UNUSED):	verhogenInt(&Semaphores.network[deviceNumber], 	deviceRegister->status);	break;
	case (INT_PRINTER):	verhogenInt(&Semaphores.printer[deviceNumber], 	deviceRegister->status);	break;
	}

	/* Acknowledge the outstanding interrupt */
	deviceRegister->command = DEV_C_ACK;
}

/*
@brief Acknowledge a pending interrupt on the terminal, distinguishing between receiving and sending ones.
@return Void.
*/
HIDDEN void intTerminal()
{
	int *deviceBitmap, deviceNumber;
	termreg_t *deviceRegister;

	/* Get the starting address of the device bitmap */
	deviceBitmap = (int *) CDEV_BITMAP_ADDR(INT_TERMINAL);

	/* Get the highest priority device affected by a pending interrupt */
	deviceNumber = getDevice(*deviceBitmap);

	/* Get the device status */
	deviceRegister = (termreg_t *) DEV_REG_ADDR(INT_TERMINAL, deviceNumber);

	/* [Case 1] Sending a character */
	if ((deviceRegister->recv_status & DEV_TERM_STATUS) == DEV_TRCV_S_CHARRECV)
	{
		/* Perform a V on the device semaphore */
		verhogenInt(&Semaphores.terminalR[deviceNumber], deviceRegister->recv_status);

		/* Identify the pending interrupt */
		deviceRegister->recv_command = DEV_C_ACK;
	}
	/* [Case 2] Receiving a character */
	else if ((deviceRegister->transm_status & DEV_TERM_STATUS) == DEV_TTRS_S_CHARTRSM)
	{
		/* Perform a V on the device semaphore */
		verhogenInt(&Semaphores.terminalT[deviceNumber], deviceRegister->transm_status);

		/* Identify the pending interrupt */
		deviceRegister->transm_command = DEV_C_ACK;
	}
}


/*
@brief The function identifies the pending interrupt and performs a V on the related Semaphores.
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

		/* Re-enable interrupts */
		/*CurrentProcess->p_s.cpsr = STATUS_ALL_INT_ENABLE(CurrentProcess->p_s.cpsr);*/
		/*SoftBlockCount++;*/
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
	if (!CurrentProcess && emptyProcQ(ReadyQueue) && ProcessCount > 0 && SoftBlockCount == 0)
		SoftBlockCount++;
	/* Call the scheduler */
	scheduler();
}
