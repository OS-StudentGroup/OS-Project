/**
@file interrupts.c
@note Interrupt Exception Handling.
*/

#include "../e/dependencies.e"

/* Interrupt Old Area */
HIDDEN state_t *InterruptOldArea = (state_t *) INT_OLDAREA;

/**
@brief Get the highest priority device affected by a pending interrupt.
@param bitmap The device bitmap.
@return Index of the device.
*/
HIDDEN int getDevice(int bitmap)
{
	if (bitmap == (bitmap | DEV_CHECK_ADDRESS_0)) return DEV_CHECK_LINE_0;
	if (bitmap == (bitmap | DEV_CHECK_ADDRESS_1)) return DEV_CHECK_LINE_1;
	if (bitmap == (bitmap | DEV_CHECK_ADDRESS_2)) return DEV_CHECK_LINE_2;
	if (bitmap == (bitmap | DEV_CHECK_ADDRESS_3)) return DEV_CHECK_LINE_3;
	if (bitmap == (bitmap | DEV_CHECK_ADDRESS_4)) return DEV_CHECK_LINE_4;
	if (bitmap == (bitmap | DEV_CHECK_ADDRESS_5)) return DEV_CHECK_LINE_5;
	if (bitmap == (bitmap | DEV_CHECK_ADDRESS_6)) return DEV_CHECK_LINE_6;
	return DEV_CHECK_LINE_7;
}

/*
@brief Performs a V on the given device semaphore.
@param semaddr Address of the semaphore.
@param status Device status.
@return Void.
*/
HIDDEN void intVerhogen(int *semaddr, int status)
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
			/* Unleash all of them */
			for (; PseudoClock < 0; PseudoClock++)
			{
				/* If there is a blocked process */
				if ((process = removeBlocked(&PseudoClock)))
				{
					/* Add the process into the Ready Queue */
					insertProcQ(&ReadyQueue, process);

					SoftBlockCount--;
				}
			}
		}
		/* [Case 1.2] There is at most one blocked process */
		else
		{
			/* [Case 1.2.1] 0 blocked processes */
			if (!(process = removeBlocked(&PseudoClock))) PseudoClock--;
			/* [Case 1.2.2] 1 blocked process */
			else
			{
				/* Add the process into the Ready Queue */
				insertProcQ(&ReadyQueue, process);

				SoftBlockCount--;
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
	else setTIMER(SCHED_PSEUDO_CLOCK - TimerTick);
}

/*
@brief Acknowledge a pending interrupt through setting the command code in the device register.
@return Void.
*/
HIDDEN void intDevice(int cause)
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
		case (INT_DISK):	intVerhogen(&Semaphores.disk[deviceNumber], 	deviceRegister->status);	break;
		case (INT_TAPE):	intVerhogen(&Semaphores.tape[deviceNumber], 	deviceRegister->status);	break;
		case (INT_UNUSED):	intVerhogen(&Semaphores.network[deviceNumber], 	deviceRegister->status);	break;
		case (INT_PRINTER):	intVerhogen(&Semaphores.printer[deviceNumber], 	deviceRegister->status);	break;
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
		intVerhogen(&Semaphores.terminalR[deviceNumber], deviceRegister->recv_status);

		/* Identify the pending interrupt */
		deviceRegister->recv_command = DEV_C_ACK;
	}
	/* [Case 2] Receiving a character */
	else if ((deviceRegister->transm_status & DEV_TERM_STATUS) == DEV_TTRS_S_CHARTRSM)
	{
		/* Perform a V on the device semaphore */
		intVerhogen(&Semaphores.terminalT[deviceNumber], deviceRegister->transm_status);

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
		saveCurrentState(InterruptOldArea, &(CurrentProcess->p_s));

		/* Decrease Program Counter */
		CurrentProcess->p_s.pc -= WORD_SIZE;
	}

	/* Get the interrupt cause and call the handler accordingly */
	interruptCause = InterruptOldArea->CP15_Cause;
	if 		(CAUSE_IP_GET(interruptCause, INT_TIMER))		intTimer();				/* Timer */
	else if (CAUSE_IP_GET(interruptCause, INT_DISK))		intDevice(INT_DISK);	/* Disk */
	else if (CAUSE_IP_GET(interruptCause, INT_TAPE))		intDevice(INT_TAPE);	/* Tape */
	else if (CAUSE_IP_GET(interruptCause, INT_UNUSED))		intDevice(INT_UNUSED);	/* Unused */
	else if (CAUSE_IP_GET(interruptCause, INT_PRINTER))		intDevice(INT_PRINTER);	/* Printer */
	else if (CAUSE_IP_GET(interruptCause, INT_TERMINAL))	intTerminal();			/* Terminal */

	/* Call the scheduler */
	scheduler();
}
