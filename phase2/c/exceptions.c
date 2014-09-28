/*
@file exceptions.c
@note System Call/Breakpoint Exception Handling
*/

#include "../e/dependencies.e"

HIDDEN state_t *SYSBP_Old = 	(state_t *) SYSBK_OLDAREA;		/* System Call/Breakpoint Old Area */
HIDDEN state_t *TLB_Old = 		(state_t *) TLB_OLDAREA;		/* TLB Old Area */
HIDDEN state_t *PGMTRAP_Old = 	(state_t *) PGMTRAP_OLDAREA;	/* Program Trap Old Area */

/**
@brief Save current state in a new state.
@param oldState Current state.
@param newState New state.
@return Void.
*/
EXTERN void saveCurrentState(state_t *oldState, state_t *newState)
{
	newState->a1 = oldState->a1;
	newState->a2 = oldState->a2;
	newState->a3 = oldState->a3;
	newState->a4 = oldState->a4;
	newState->v1 = oldState->v1;
	newState->v2 = oldState->v2;
	newState->v3 = oldState->v3;
	newState->v4 = oldState->v4;
	newState->v5 = oldState->v5;
	newState->v6 = oldState->v6;
	newState->sl = oldState->sl;
	newState->fp = oldState->fp;
	newState->ip = oldState->ip;
	newState->sp = oldState->sp;
	newState->lr = oldState->lr;
	newState->pc = oldState->pc;
	newState->cpsr = oldState->cpsr;
	newState->CP15_Control = oldState->CP15_Control;
	newState->CP15_EntryHi = oldState->CP15_EntryHi;
	newState->CP15_Cause = oldState->CP15_Cause;
	newState->TOD_Hi = oldState->TOD_Hi;
	newState->TOD_Low = oldState->TOD_Low;
}

/**
@brief The function will undertake one of the following actions:
(1) If the offending process has NOT issued a SYS5, then invoke SYS2;
(2) If the offending process has issued a SYS5, then pass up the exception.
@return Void.
*/
HIDDEN void checkSYS5(int exceptionType, state_t *exceptionOldArea)
{
	/* [Case 1] SYS5 has not been issued */
	if (CurrentProcess->exceptionState[exceptionType] == 0)
	{
		/* Terminate the current process (SYS2) */
		terminateProcess();

		/* Call the scheduler */
		scheduler();
	}
	/* [Case 2] SYS5 has been issued */
	else
	{
		/* Move current process Exception State Area into the processor Exception State Area */
		saveCurrentState(exceptionOldArea, CurrentProcess->p_stateOldArea[exceptionType]);

		/* Load the processor state in order to start execution */
		LDST(CurrentProcess->p_stateNewArea[exceptionType]);
	}
}

/**
@brief This function handles a system call request coming from a process running in User Mode.
@return Void.
*/
HIDDEN void syscallUserMode()
{
	/* [Case 1] A privileged system call has been raised */
	if (SYSBP_Old->a1 > 0 && SYSBP_Old->a1 < 9)
	{
		/* Save SYS/BP Old Area into PgmTrap Old Area */
		saveCurrentState(SYSBP_Old, PGMTRAP_Old);

		/* Set program trap cause to RI (Reserved Instruction) */
		PGMTRAP_Old->CP15_Cause = CAUSE_EXCCODE_SET(PGMTRAP_Old->CP15_Cause, EXC_RESERVEDINSTR);

		/* Call the trap handler in order to trigger the PgmTrap exception response */
		pgmTrapHandler();
	}
	/* [Case 2] A non privileged system call has been raised */
	else
		/* Distinguish whether SYS5 has been invoked or not */
		checkSYS5(SYSBK_EXCEPTION, SYSBP_Old);
}

/**
@brief This function handles a system call request coming from a process running in Kernel Mode.
@return Void.
*/
HIDDEN void syscallKernelMode()
{
	/* Identify and handle the system call */
	switch (SYSBP_Old->a1)
	{
		case CREATEPROCESS:
			CurrentProcess->p_s.a1 = createProcess((state_t *) SYSBP_Old->a2);
			break;

		case TERMINATEPROCESS:
			terminateProcess();
			break;

		case VERHOGEN:
			verhogen((int *) SYSBP_Old->a2);
			break;

		case PASSEREN:
			passeren((int *) SYSBP_Old->a2);
			break;

		case GETCPUTIME:
			CurrentProcess->p_s.a1 = getCPUTime();
			break;

		case WAITCLOCK:
			waitClock();
			break;

		case WAITIO:
			CurrentProcess->p_s.a1 = waitIO((int) SYSBP_Old->a2, (int) SYSBP_Old->a3, (int) SYSBP_Old->a4);
			break;

		case SPECTRAPVEC:
			specTrapVec((int) SYSBP_Old->a2, (state_t *) SYSBP_Old->a3, (state_t *) SYSBP_Old->a4);
			break;

		default:
			/* Distinguish whether SYS5 has been invoked or not */
			checkSYS5(SYSBK_EXCEPTION, SYSBP_Old);
	}

	/* Call the scheduler */
	scheduler();
}

/**
@brief This function handles SYSCALL or Breakpoint exceptions, which occurs when a SYSCALL or BREAK assembler
instruction is executed.
@return Void.
*/
EXTERN void sysBpHandler()
{
	/* Save SYS/BP Old Area state */
	saveCurrentState(SYSBP_Old, &(CurrentProcess->p_s));

	/* Select handler accordingly to the exception type */
	switch (CAUSE_EXCCODE_GET(SYSBP_Old->CP15_Cause))
	{
		/* [Case 1] The exception is a system call */
		case EXC_SYSCALL:
			/* Distinguish between User Mode and Kernel Mode */
			((SYSBP_Old->cpsr & STATUS_SYS_MODE) == STATUS_USER_MODE)? syscallUserMode() : syscallKernelMode();
			break;

		/* [Case 2] The exception is a breakpoint */
		case EXC_BREAKPOINT:
			/* Distinguish whether SYS5 has been invoked or not */
			checkSYS5(SYSBK_EXCEPTION, SYSBP_Old);
			break;

		default: PANIC(); /* Anomaly */
	}
}

/*
@brief Program Trap handler.
@return Void.
*/
EXTERN void pgmTrapHandler()
{
	/* If a process is running, load Program Trap Old Area into the Current Process state */
	(CurrentProcess)? saveCurrentState(PGMTRAP_Old, &(CurrentProcess->p_s)) : PANIC(); /* Anomaly */

	/* Distinguish whether SYS5 has been invoked or not */
	checkSYS5(PGMTRAP_EXCEPTION, PGMTRAP_Old);
}

/*
@brief TLB (Translation Look Aside Buffer) handler.
@return Void.
*/
EXTERN void tlbHandler()
{
	/* If a process is running, load Program Trap Old Area into the Current Process state */
	(CurrentProcess)? saveCurrentState(TLB_Old, &(CurrentProcess->p_s)) : PANIC(); /* Anomaly */

	/* Distinguish whether SYS5 has been invoked or not */
	checkSYS5(TLB_EXCEPTION, TLB_Old);
}

/**
@brief (SYS1) Creates a new process.
@param state Processor state from which create a new process.
@return -1 in case of failure; 0 in case of success.
*/
EXTERN int createProcess(state_t *state)
{
	pcb_t *process;

	/* Allocate a new PCB */
	if (!(process = allocPcb())) return -1; /* Failure */

	/* Load processor state into process state */
	saveCurrentState(state, &(process->p_s));

	/* Update process counter, process tree and process queue */
	ProcessCount++;
	insertChild(CurrentProcess, process);
	insertProcQ(&ReadyQueue, process);

	return 0; /* Success */
}

/**
@brief Recursively terminates a process and its progeny.
@param process Pointer to the Process Control Block.
@return Void.
*/
HIDDEN void _terminateProcess(pcb_t *process)
{
	/* As long as the process has children, remove the first one and perform a recursive call */
	while (!emptyChild(process)) _terminateProcess(removeChild(process));

	/* If the process is blocked on a semaphore */
	if (process->p_semAdd)
	{
		/* If it is the Pseudo-Clock semaphore or a non-device semaphore, and its value is negative */
		if ((process->p_semAdd == &PseudoClock || !process->p_isBlocked) && (*process->p_semAdd) < 0)
			(*process->p_semAdd)++; /* Update the value */

		/* Extract the process from the semaphore */
		if (!outBlocked(process)) PANIC(); /* Anomaly */

		/* In case of a device semaphore, update the Soft Block Count */
		if (process->p_isBlocked) SoftBlockCount--;
	}

	/* Decrease the number of active processes */
	ProcessCount--;

	/* Insert the process block into the pcbFree list */
 	freePcb(process);
}

/**
@brief (SYS2) Terminates a process and all its progeny.
@return Void.
*/
EXTERN void terminateProcess()
{
	/* There is not a running process */
	if (!CurrentProcess) return;

	/* Make the current process block no longer the child of its parent. */
	outChild(CurrentProcess);

	/* Call recursive function */
	_terminateProcess(CurrentProcess);

	/* Now there is no more a running process */
	CurrentProcess = NULL;
}

/**
@brief Verhogen (V) (SYS3). Performs a V operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
EXTERN void verhogen(int *semaddr)
{
	pcb_t *process;

	/* Perform a V on the semaphore */
	(*semaddr)++;

	/* If ASL is not empty */
	if ((process = removeBlocked(semaddr)))
	{
		/* Insert process into the ready queue */
		insertProcQ(&ReadyQueue, process);
		process->p_isBlocked = FALSE;
	}
}


/**
@brief Passeren (P) (SYS4). Performs a P operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
EXTERN void passeren(int *semaddr)
{
	/* If semaphore value becomes negative */
	if (--(*semaddr) < 0)
	{
		/* Block process into the semaphore queue */
		if (insertBlocked(semaddr, CurrentProcess)) PANIC(); /* Anomaly */
		CurrentProcess = NULL;

		/* Call the scheduler */
		scheduler();
	}
}

/**
@brief Performs a P operation on a device semaphore.
Thus, increments the Soft Block Count and set the p_isBlocked flag as TRUE.
@param semaddr Semaphore address.
@return Void.
*/
HIDDEN void devPasseren(int *semaddr)
{
	/* If semaphore value becomes negative */
	if (--(*semaddr) < 0)
	{
		/* Block process into the semaphore queue */
		if (insertBlocked(semaddr, CurrentProcess)) PANIC(); /* Anomaly */
		SoftBlockCount++;
		CurrentProcess->p_isBlocked = TRUE;
		CurrentProcess = NULL;

		/* Call the scheduler */
		scheduler();
	}
}

/**
@brief (SYS5) Specify Exception State Vector.
@param type Type of exception.
@param stateOld The address into which the old processor state is to be stored when an exception
occurs while running this process.
@param stateNew The processor state area that is to be taken as the new processor state if an
exception occurs while running this process.
@return Void.
*/
EXTERN void specTrapVec(int type, state_t *stateOld, state_t *stateNew)
{
	/* [Case 1] If the exception type is not recognized or SYS5 has been called more than once, terminate the process */
	if (type < 0 || type > 2 || ++CurrentProcess->exceptionState[type] > 1) terminateProcess();
	/* [Case 2] SYS5 has been called for the first time */
	else
	{
		CurrentProcess->p_stateOldArea[type] = stateOld;
		CurrentProcess->p_stateNewArea[type] = stateNew;
	}
}

/**
@brief (SYS6) Retrieve the CPU time of the current process.
@return CPU time of the current process.
*/
EXTERN U32 getCPUTime()
{
	/* Perform a last update of the CPU time */
	CurrentProcess->p_cpu_time += getTODLO() - ProcessTOD;
	ProcessTOD = getTODLO();

	return CurrentProcess->p_cpu_time;
}

/**
@brief (SYS7) Performs a P on the pseudo-clock semaphore.
@return Void.
*/
EXTERN void waitClock()
{
	devPasseren(&PseudoClock);
}

/**
@brief (SYS8) Perform a P operation on a device semaphore.
@param interruptLine Interrupt line.
@param deviceNumber Device number.
@param reading : TRUE in case of reading; FALSE else.
@return Return the device's Status Word.
*/
EXTERN unsigned int waitIO(int interruptLine, int deviceNumber, int reading)
{
	U32 status;

	switch (interruptLine)
	{
		case INT_DISK:		devPasseren(&Semaphores.disk[deviceNumber]);		break;
		case INT_TAPE:		devPasseren(&Semaphores.tape[deviceNumber]);		break;
		case INT_UNUSED:	devPasseren(&Semaphores.network[deviceNumber]);		break;
		case INT_PRINTER:	devPasseren(&Semaphores.printer[deviceNumber]);		break;
		case INT_TERMINAL:	devPasseren((reading)? 	&Semaphores.terminalR[deviceNumber] :
													&Semaphores.terminalT[deviceNumber]); break;
		default: PANIC(); /* Anomaly */
	}

	/* [Case 1] The device is not the terminal */
	if (interruptLine != INT_TERMINAL)
		status = ((dtpreg_t *) DEV_REG_ADDR(interruptLine, deviceNumber))->status;
	/* [Case 2] The device is the terminal */
	else
	{
		termreg_t *terminal = (termreg_t *) DEV_REG_ADDR(interruptLine, deviceNumber);

		/* Distinguish between receiving and transmitting */
		status = (reading)? terminal->recv_status : terminal->transm_status;
	}

	return status;
}
