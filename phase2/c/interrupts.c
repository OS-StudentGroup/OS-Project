/**
@file interrupts.c
@note Interrupt Exception Handling.
*/

#include "../e/inclusions.e"

/* Interrupt Old Area */
HIDDEN state_t *interruptOldArea = (state_t *) INT_OLDAREA;

/* Device Base Address */
HIDDEN memaddr deviceBaseAddress;

/**
@brief Identify at which device a pending interrupt refers to.
@param bitmap Bitmap of the pending interrupt.
@return The device identifier.
*/
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
	default:				return IL_TERMINAL;
	}
}

/**
@brief Performs a V on the given device semaphore.
@param semaddr Address of the semaphore.
@param status Device status.
@return The unblocked process, or NULL.
*/
HIDDEN pcb_t *verhogenInt(int *semaddr, int status)
{
	pcb_t *process = NULL;
	
	/* Performs a V on the semaphore */
	(*semaddr)++;

	/* [Case 1] At least one blocked process */
	if (!(process = removeBlocked(semaddr)))
	{
		/* Add the process into the Ready Queue */
		insertProcQ(&readyQueue, process);

		softBlockCount--;
		p->p_s.a1 = status;
		process->p_isOnSem = FALSE;
	}
	
	return process;
}

/**
@brief Acknowledge a pending interrupt on the Timer Click.
@return Void.
*/
HIDDEN void intTimer()
{
	pcb_t *process;

	/* Update elapsed time */
	timerTick += getTODLO() - startTimerTick;
	startTimerTick = getTODLO();

	/* [Case 1] The time slice for the current process did not ran out */
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

					process->p_isOnSem = FALSE;
					softBlockCount--;
				}
			}
		}
		/* [Case 2] At most one blocked process */
		else
		{
			/* [Case 2.1] 0 blocked processes */
			if (!(process = removeBlocked(&pseudo_clock)))
				/* Perform a P on the pseudo-clock */
				pseudo_clock--;
			/* [Case 2.2] 1 blocked process */
			else
			{
				/* Add the process into the Ready Queue */
				insertProcQ(&readyQueue, process);

				process->p_isOnSem = FALSE;
				softBlockCount--;

				/* Perform a V on the pseudo-clock */
				pseudo_clock++;
			}
		}

		/* Reset the timer tick used to compute the pseudo-clock tick */
		timerTick = 0;
		startTimerTick = getTODLO();
	}
	/* [Case 2] The time slice for the current process ran out */
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
	/* [Case 3] There is no a running process */
	else
		setTIMER(SCHED_PSEUDO_CLOCK - timerTick);
}

/**
@brief Acknowledge a pending interrupt through setting the command code in the device register.
@return Void.
*/
HIDDEN void devInterrupt(int cause)
{
	int *deviceBitmap, deviceNumber;
	pcb_t *process;
	dtpreg_t *status;

	/* Get the starting address of the device bitmap */
	deviceBitmap = (int *) CDEV_BITMAP_ADDR(cause);

	/* Get the highest priority device with pending interrupt */
	deviceNumber = getDevice(*deviceBitmap);

	/* Get the device status */
	status = (dtpreg_t *) DEV_REG_ADDR(cause, deviceNumber);

	/* Perform a V on the device semaphore */
	switch (cause)
	{
	case (INT_DISK):	process = verhogenInt(&sem.disk[deviceNumber], status->status);		break;
	case (INT_TAPE):	process = verhogenInt(&sem.tape[deviceNumber], status->status);		break;
	case (INT_UNUSED):	process = verhogenInt(&sem.network[deviceNumber], status->status);	break;
	case (INT_PRINTER):	process = verhogenInt(&sem.printer[deviceNumber], status->status);	break;
	}

	/* Ack on device to identify the pending interrupt */
	status->command = DEV_C_ACK;
}

/**
@brief Acknowledge a pending interrupt on the terminal, distinguishing between receiving and sending interrupts.
@return Void.
*/
HIDDEN void intTerminal()
{
	int *deviceBitmap, deviceNumber;
	pcb_t *process;
	termreg_t *status;

	/* Get the starting address of the device bitmap */
	deviceBitmap = (int *) CDEV_BITMAP_ADDR(INT_TERMINAL);

	/* Get the highest priority device with pending interrupt */
	deviceNumber = getDevice(*deviceBitmap);

	/* Get the device status */
	status = (termreg_t *) DEV_REG_ADDR(INT_TERMINAL, deviceNumber);

	/* [Case 1] Sending a character */
	if ((status->recv_status & DEV_TERM_STATUS) == DEV_TRCV_S_CHARRECV)
	{
		/* Perform a V on the device semaphore */
		verhogenInt(&sem4.terminalR[deviceNumber], status->recv_status);

		/* Ack on device to identify the pending interrupt */
		status->recv_command = DEV_C_ACK;
	}
	/* [Case 2] Receiving a character */
	else if ((status->transm_status & DEV_TERM_STATUS) == DEV_TTRS_S_CHARTRSM)
	{
		/* Perform a V on the device semaphore */
		verhogenInt(&sem4.terminalT[deviceNumber], status->transm_status);

		/* Ack on device to identify the pending interrupt */
		status->transm_command = DEV_C_ACK;
	}
}


/**
@brief The function identifies the pending interrupt and performs a V on the related semaphore.
@return Void.
*/
void intHandler()
{
	int intCause;
	
	/* If there is a running process */
	if (currentProcess)
	{
		/* Save current process state */
		saveCurrentState(interruptOldArea , &(currentProcess->p_s));

		/* Decrease Program Counter */
		currentProcess->p_s.pc -= 2 * WORD_SIZE;
	}

	/* Get interrupt cause */
	intCause = interruptOldArea->CP15_Cause;
	if 	(CAUSE_IP_GET(intCause, INT_TIMER))	intTimer();			/* Timer */
	else if (CAUSE_IP_GET(intCause, INT_DISK))	devInterrupt(INT_DISK);		/* Disk */
	else if (CAUSE_IP_GET(intCause, INT_TAPE))	devInterrupt(INT_TAPE);		/* Tape */
	else if (CAUSE_IP_GET(intCause, INT_UNUSED))	devInterrupt(INT_UNUSED);	/* Unsused */
	else if (CAUSE_IP_GET(intCause, INT_PRINTER))	devInterrupt(INT_PRINTER);	/* Printer */
	else if (CAUSE_IP_GET(intCause, INT_TERMINAL))	intTerminal();			/* Terminal */
	
	/* Run scheduler */
	scheduler();
}
