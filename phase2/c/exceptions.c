/*
@file exceptions.c
@note SYS/Bp Exception Handling
*/

#include "../e/dependencies.e"
#include "../../include/types.h"

HIDDEN state_t *SYSBP_Old = (state_t *) SYSBK_OLDAREA;		/* System Call/Break Point Old Area */
HIDDEN state_t *TLB_Old = (state_t *) TLB_OLDAREA;			/* TLB Old Area */
HIDDEN state_t *PGMTRAP_Old = (state_t *) PGMTRAP_OLDAREA;	/* Program Trap Old Area */

/**
@brief Save current state in a new state.
@param currentState Current state.
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
@brief For Breakpoint and System Call higher than 9 the exception handler will
take one of the following actions:
(1) If the offending process has NOT issued a SYS5, then invoke SYS2;
(2) If the offending process has issued a SYS5, then pass up the exception.
@return Void.
*/
HIDDEN void checkSYS5(int exceptionType, state_t *exceptionOldArea)
{
	/* If SYS5 has not been yet executed, then the current process shall terminate */
	if (CurrentProcess->exceptionState[exceptionType] == 0)
	{
		/* Terminate the current process */
		terminateProcess(); /* (SYS2) */

		/* Anomaly */
		if (CurrentProcess)
			CurrentProcess->p_s.a1 = -1;
		if (!CurrentProcess && emptyProcQ(ReadyQueue) && ProcessCount > 0 && SoftBlockCount == 0) tprint("CAZZOe9");
		/* Call the scheduler */
		scheduler();
	}
	else
	{
		/* Move current process' exception state area into the processor's exception state area */
		saveCurrentState(exceptionOldArea, CurrentProcess->p_stateOldArea[exceptionType]);

		/* Load the processor state in order to start execution */
		LDST(CurrentProcess->p_stateNewArea[exceptionType]);
	}
}

/*
@brief The eight nucleus services are considered privileged services and are only
available to processes executing in kernel-mode. Any attempt to request one of
these services while in user-mode should trigger a PgmTrap exception response.
*/
HIDDEN void systemCallUserMode()
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
		/* Distinguish whether SYS5 has been invoked before or not */
		checkSYS5(SYSBK_EXCEPTION, SYSBP_Old);
}

HIDDEN void systemCallKernelMode()
{
	/* Get SYSCALL parameters */
	U32 a2 = SYSBP_Old->a2;
	U32 a3 = SYSBP_Old->a3;
	U32 a4 = SYSBP_Old->a4;

	/* Identify and handle the system call */
	switch (SYSBP_Old->a1)
	{
		/* [Case 1] Create a new process */
		case CREATEPROCESS:
			CurrentProcess->p_s.a1 = createProcess((state_t *) a2);
			break;

		/* [Case 2] Terminate the current process */
		case TERMINATEPROCESS:
			terminateProcess(); /* (SYS2) */

			/* Anomaly */
			if (CurrentProcess)
				CurrentProcess->p_s.a1 = -1;
			break;

		/* [Case 3] Perform a V operation on a semaphore */
		case VERHOGEN:
			verhogen((int *) a2);
			break;

		/* [Case 4] Perform a P operation on a semaphore */
		case PASSEREN:
			passeren((int *) a2);
			break;

		/* [Case 5] Get the CPU time */
		case GETCPUTIME:
			CurrentProcess->p_s.a1 = getCPUTime();
			break;

		case WAITCLOCK:
			waitClock();
			break;

		case WAITIO:
			CurrentProcess->p_s.a1 = waitIO((int) a2, (int) a3, (int) a4);
			break;

		case SPECTRAPVEC:
			specTrapVec((int) a2, (state_t *) a3, (state_t *) a4);
			break;

		default:
			/* Distinguish whether SYS5 has been invoked before or not */
			checkSYS5(SYSBK_EXCEPTION, SYSBP_Old);
	}
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
			((SYSBP_Old->cpsr & STATUS_SYS_MODE) == STATUS_USER_MODE)? systemCallUserMode() : systemCallKernelMode();
			break;

		/* [Case 2] The exception is a breakpoint */
		case EXC_BREAKPOINT:
			/* Distinguish whether SYS5 has been invoked before or not */
			checkSYS5(SYSBK_EXCEPTION, SYSBP_Old);
			break;

		default:
			PANIC(); /* Anomaly */
	}
	/*if (!CurrentProcess && emptyProcQ(ReadyQueue) && ProcessCount > 0 && SoftBlockCount == 0) tprint("CAZZO sysBpHandler/n");*/
	/* Call the scheduler */
	scheduler();
}

/*
@brief Program Trap handler.
@return Void.
*/
EXTERN void pgmTrapHandler()
{
	/* If a process is running, load Program Trap Old Area into the current process state, otherwise call the scheduler */
	(CurrentProcess)? saveCurrentState(PGMTRAP_Old, &(CurrentProcess->p_s)) : scheduler();

	/* Distinguish whether SYS5 has been invoked before or not */
	checkSYS5(PGMTRAP_EXCEPTION, PGMTRAP_Old);
}

/*
@brief TLB (Translation Look Aside Buffer) Exception Handling.
@return Void.
*/
EXTERN void tlbHandler()
{
	/* If a process is running, load Program Trap Old Area into the current process state, otherwise call the scheduler */
	(CurrentProcess)? saveCurrentState(TLB_Old, &(CurrentProcess->p_s)) : scheduler();

	/* Distinguish whether SYS5 has been invoked before or not */
	checkSYS5(TLB_EXCEPTION, TLB_Old);
}

/**
@brief (SYS1) Create a new process.
@param state Processor state from which create a new process.
@return -1 in case of failure; PID in case of success.
*/
EXTERN int createProcess(state_t *state)
{
	pcb_t *process;

	/* Allocate a new PCB */
	if (!(process = allocPcb()))
		return -1; /* Failure */

	/* Load processor state into process state */
	saveCurrentState(state, &(process->p_s));

	/* Update process counter, process tree and process queue */
	ProcessCount++;
	insertChild(CurrentProcess, process);
	insertProcQ(&ReadyQueue, process);

	return ProcessCount;
}

/*
@brief Recursively terminate a process and its progeny.
@param process Pointer to the process control block.
@return Void.
*/
HIDDEN void _terminateProcess(pcb_t *process)
{
	/* As long as the process has children */
	while (!emptyChild(process))
		/* Remove the first one and perform a recursive call */
		_terminateProcess(removeChild(process));

	/* Process blocked on a semaphore */
	if (process->p_semAdd)
	{
		/* [Case 1] Process blocked on a device semaphore */
		if (process->p_isBlocked)
		{
			/* verhogen(process->p_semAdd);*/
			/* Performs a V on the semaphore */
			(*process->p_semAdd)++;

			/* Extract the process from the semaphore */
			outBlocked(process);
			SoftBlockCount--;
		}
		/* [Case 2] Process blocked on the Pseudo-Clock semaphore */
		if (process->p_semAdd == &PseudoClock)
			PseudoClock++;
	}

	/* Decrease the number of active processes */
	ProcessCount--;

	/* Insert the process block into the pcbFree list */
 	freePcb(process);
}

/**
@brief (SYS2) Terminate a process and all its progeny. In case a process in blocked on a semaphore, performs a V on it.
@return 0 in case of success; -1 in case of error.
*/
EXTERN void terminateProcess()
{
	/* There is not a running process */
	if (!CurrentProcess) return;

	/* Make the current process block no longer the child of its parent. */
	outChild(CurrentProcess);

	/* Call recursive function */
	_terminateProcess(CurrentProcess);

	CurrentProcess = NULL;
}

/*
@brief Verhogen (V) (SYS3). Perform a V operation on a semaphore.
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
		/*SoftBlockCount--;*/
	}
}


/*
@brief Passeren (P) (SYS4). Perform a P operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
EXTERN void passeren(int *semaddr)
{
	/* If semaphore value becomes negative */
	if (--(*semaddr) < 0)
	{
		/* Block process into the semaphore queue */
		if (insertBlocked(semaddr, CurrentProcess))
			PANIC(); /* Anomaly */

		CurrentProcess->p_isBlocked = TRUE;
		CurrentProcess = NULL;
		/* SoftBlockCount++;*/
	}
}

/**
@brief Performs a P on a device semaphore and increments the Soft-Block count, i.e. increase
by one the number of processes in the system currently blocked and waiting for an interrupt.
@param semaddr Semaphore's address.
@return Void.
*/
EXTERN void passerenIO(int *semaddr)
{
	/* Perform a P on the semaphore */
	if (--(*semaddr) < 0)
	{
		/* Block current process in the semaphore queue */
		if (insertBlocked(semaddr, CurrentProcess))
			PANIC(); /* Anomaly */

		CurrentProcess->p_isBlocked = TRUE;
		CurrentProcess = NULL;
		SoftBlockCount++;
		/*if(SoftBlockCount == 0)tprint("CAZZO999");*/
		/* Call the scheduler */
		scheduler();
	}
}

/*
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
	/* [Case 1] If the exception type is not recognized or SYS5 has been called more than once */
	if ((type < 0 || type > 2) || ++CurrentProcess->exceptionState[type] > 1)
	{
		/* Terminate the process */
		terminateProcess(); /* (SYS2) */

		/* Anomaly */
		if (CurrentProcess)
			CurrentProcess->p_s.a1 = -1;
	}
	/* [Case 2] SYS5 has been called for the first time */
	else
	{
		/* Save a2 and a3 to facilitate the respective exception */
		CurrentProcess->p_stateOldArea[type] = stateOld;
		CurrentProcess->p_stateNewArea[type] = stateNew;
	}
	if (!CurrentProcess && emptyProcQ(ReadyQueue) && ProcessCount > 0 && SoftBlockCount == 0)
		SoftBlockCount++;
	/* Call the scheduler */
	scheduler();
}

/*
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

/*
@brief (SYS7) Performs a P on the pseudo-clock semaphore.
@return Void.
*/
EXTERN void waitClock()
{
	/* Just call the Passeren method */
	passerenIO(&PseudoClock);
}

/*
@brief (SYS8) Perform a P operation on a device semaphore.
@param interruptLine Interrupt line.
@param deviceNumber Device number.
@param reading : TRUE in case of reading; FALSE else.
@return Return the device's Status Word.
*/
EXTERN unsigned int waitIO(int interruptLine, int deviceNumber, int reading)
{
	switch (interruptLine)
	{
		case INT_DISK:		passerenIO(&Semaphores.disk[deviceNumber]);		break;
		case INT_TAPE:		passerenIO(&Semaphores.tape[deviceNumber]);		break;
		case INT_UNUSED:	passerenIO(&Semaphores.network[deviceNumber]);	break;
		case INT_PRINTER:	passerenIO(&Semaphores.printer[deviceNumber]);	break;
		case INT_TERMINAL:	passerenIO((reading)?
								&Semaphores.terminalR[deviceNumber] :
								&Semaphores.terminalT[deviceNumber]);		break;
		default: PANIC(); /* Anomaly */
	}

	/* [Case 1] The device is not the terminal */
	if (interruptLine != INT_TERMINAL)
	{
		dtpreg_t *device = (dtpreg_t *) DEV_REG_ADDR(interruptLine, deviceNumber);
		return device->status;
	}
	/* [Case 2] The device is the terminal */
	else
	{
		termreg_t *terminal = (termreg_t *) DEV_REG_ADDR(interruptLine, deviceNumber);

		/* Distinguish between receiving and transmitting */
		return (reading)? terminal->recv_status : terminal->transm_status;
	}

	PANIC(); /* Anomaly */
	return 0;
}
