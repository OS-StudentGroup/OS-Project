/*
@file exceptions.c
@note SYS/Bp Exception Handling
*/

#include "../../phase1/h/asl.h"
#include "../../phase1/e/pcb.e"
#include "../e/exceptions.e"
#include "../h/initial.h"
#include "../e/interrupts.e"
#include "../e/scheduler.e"
#include "../../include/libuarm.h"

HIDDEN state_t *SysBP_old = (state_t *) SYSBK_OLDAREA;		/* System Call/Break Point Old Area */
HIDDEN state_t *TLB_old = (state_t *) TLB_OLDAREA;			/* TLB Old Area */
HIDDEN state_t *PgmTrap_old = (state_t *) PGMTRAP_OLDAREA;	/* Program Trap Old Area */

/**
@brief Save current state in a new state.
@param currentState Current state.
@param newState New state.
@return Void.
*/
EXTERN void saveCurrentState(state_t *currentState, state_t *newState)
{
	newState->a1 = currentState->a1;
	newState->a2 = currentState->a2;
	newState->a3 = currentState->a3;
	newState->a4 = currentState->a4;
	newState->v1 = currentState->v1;
	newState->v2 = currentState->v2;
	newState->v3 = currentState->v3;
	newState->v4 = currentState->v4;
	newState->v5 = currentState->v5;
	newState->v6 = currentState->v6;
	newState->sl = currentState->sl;
	newState->fp = currentState->fp;
	newState->ip = currentState->ip;
	newState->sp = currentState->sp;
	newState->lr = currentState->lr;
	newState->pc = currentState->pc;
	newState->cpsr = currentState->cpsr;
	newState->CP15_Control = currentState->CP15_Control;
	newState->CP15_EntryHi = currentState->CP15_EntryHi;
	newState->CP15_Cause = currentState->CP15_Cause;
	newState->TOD_Hi = currentState->TOD_Hi;
	newState->TOD_Low = currentState->TOD_Low;
}

/**
@brief Performs a P on a device semaphore and increments the Soft-Block count, i.e. increase
by one the number of processes in the system currently blocked and waiting for an interrupt.
@param semaddr Semaphore's address.
@return Void.
*/
HIDDEN void passerenIO(int *semaddr)
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

		/* Call the scheduler */
		scheduler();
	}
}

/**
@brief This function handles SYSCALL, distinguishing between User Mode and Kernel Mode.
@return Void.
*/
HIDDEN void systemCallHandler(int exceptionType, int statusMode)
{
	/* [Case 1] User Mode is on */
	if (statusMode == STATUS_USER_MODE)
	{
		/* [Case 1.1] One of the available system calls has been raised */
		if (SysBP_old->a1 > 0 && SysBP_old->a1 <= 8)
		{
			/* Save SYS/BP Old Area into PgmTrap Old Area */
			saveCurrentState(SysBP_old, PgmTrap_old);

			/* Set program trap cause to RI (Reserved Instruction) */
			PgmTrap_old->CP15_Cause = CAUSE_EXCCODE_SET(PgmTrap_old->CP15_Cause, EXC_RESERVEDINSTR);

			/* Call the trap handler in order to trigger the PgmTrap exception response */
			pgmTrapHandler();
		}
		/* [Case 1.2] No one of the available system calls has been raised */
		else
		{
			/* [Case 1.2.1] SYS5 has not been executed */
			if (CurrentProcess->exceptionState[SYS_BK_EXCEPTION] == 0)
			{
				/* Terminate the current process */
				terminateProcess();

				/* Anomaly */
				if (CurrentProcess)
					CurrentProcess->p_s.a1 = -1;

				/* Call the scheduler */
				scheduler();
			}
			/* [Case 1.2.1] SYS5 has been executed */
			else
			{
				/* Save SYS/BP Old Area in the current process Old Area*/
				saveCurrentState(SysBP_old, CurrentProcess->p_stateOldArea[SYS_BK_EXCEPTION]);

				/* Load the processor state in order to start execution */
				LDST(CurrentProcess->p_stateNewArea[SYS_BK_EXCEPTION]);
			}
		}
	}
	/* [Case 2] Kernel Mode on */
	else
	{
		/* Get SYSCALL parameters */
		U32 a2 = SysBP_old->a2;
		U32 a3 = SysBP_old->a3;
		U32 a4 = SysBP_old->a4;

		/* Handle the system call */
		switch (SysBP_old->a1)
		{
			/* [Case 1] Create a new process */
			case CREATEPROCESS:
				CurrentProcess->p_s.a1 = createProcess((state_t *) a2);
				break;
			/* [Case 2] Terminate the current process */
			case TERMINATEPROCESS:
				terminateProcess();

				/* Anomaly */
				if (CurrentProcess)
					CurrentProcess->p_s.a1 = -1;

				/* Call the scheduler */
				scheduler();
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
				/* If SYS5 has not been yet executed, the current process shall terminate */
				if (CurrentProcess->exceptionState[SYS_BK_EXCEPTION] == 0)
				{
					/* Terminate the current process */
					terminateProcess();

					/* Anomaly */
					if (CurrentProcess)
						CurrentProcess->p_s.a1 = -1;

					/* Call the scheduler */
					scheduler();
				}
				else
				{
					/* Otherwise save SYS/BP Old Area in the current process Old Area */
					saveCurrentState(SysBP_old, CurrentProcess->p_stateOldArea[SYS_BK_EXCEPTION]);

					/* Load the processor state in order to start execution */
					LDST(CurrentProcess->p_stateNewArea[SYS_BK_EXCEPTION]);
				}
		}

		/* Call the scheduler */
		scheduler();
	}
}

/**
@brief This function handles Breakpoint exceptions, distinguishing the case in which the
SYS5 has not been yet executed, and the case in which it has.
@return Void.
*/
HIDDEN void breakPointHandler(int exceptionType, int statusMode)
{
	/* If SYS5 has not been yet executed, then the current process shall terminate */
	if (CurrentProcess->exceptionState[SYS_BK_EXCEPTION] == 0)
	{
		/* Terminate the current process */
		terminateProcess();

		/* Anomaly */
		if (CurrentProcess)
			CurrentProcess->p_s.a1 = -1;

		/* Call the scheduler */
		scheduler();
	}
	else
	{
		/* Save SYS/BP Old Area in the current process */
		saveCurrentState(SysBP_old, CurrentProcess->p_stateOldArea[SYS_BK_EXCEPTION]);

		/* Load the processor state in order to start execution */
		LDST(CurrentProcess->p_stateNewArea[SYS_BK_EXCEPTION]);
	}
}

/**
@brief This function handles SYSCALL or Breakpoint exceptions, which occurs when a SYSCALL or BREAK assembler
instruction is executed.
@return Void.
*/
EXTERN void sysBpHandler()
{
	int exceptionType, statusMode;

	/* Save SYS/BP Old Area state */
	saveCurrentState(SysBP_old, &(CurrentProcess->p_s));

	/* Avoid endless loop of system calls */
	CurrentProcess->p_s.pc += 4;

	/* Get status mode of the SYS/BP Old Area */
	statusMode = SysBP_old->cpsr & STATUS_SYS_MODE;

	/* Get exception type */
	exceptionType = CAUSE_EXCCODE_GET(SysBP_old->CP15_Cause);

	/* [Case 1] It's a system call */
	if (exceptionType == EXC_SYSCALL)
		systemCallHandler(exceptionType, statusMode);
	/* [Case 2] It's a breakpoint */
	else if (exceptionType == EXC_BREAKPOINT)
		breakPointHandler(exceptionType, statusMode);
	/* Anomaly */
	else
		PANIC();
}

/**
@brief (SYS1) Create a new process.
@param statep Processor state from which create a new process.
@return -1 in case of failure; PID in case of success.
*/
EXTERN int createProcess(state_t *statep)
{
	int i;
	pcb_t *p;

	/* Allocate a new PCB */
	if (!(p = allocPcb()))
		return -1;

	/* Load processor state into process state */
	saveCurrentState(statep, &(p->p_s));

	/* Update process and Pid counter */
	ProcessCount++;
	PidCount++;
	p->p_pid = PidCount;

	/* Update process tree and process queue */
	insertChild(CurrentProcess, p);
	insertProcQ(&ReadyQueue, p);

	return PidCount;
}

/*
@brief Recursively terminate a process and its progeny.
@param process Pointer to the process control block.
@return Void.
*/
HIDDEN void _terminateProcess(pcb_t *process)
{
	pcb_t *progeny;

	/* As long as there are child processes */
	while (!emptyChild(process))
	{
		/* Remove first child */
		progeny = removeChild(process);

		/* Recursive call */
		_terminateProcess(progeny);
	}

	/* Process blocked on a semaphore */
	if (process->p_semAdd) 
	{
		/* [Case 1] Process blocked on a device semaphore */
		if (process->p_isBlocked)
		{
			/* Performs a V on the semaphore */
			(*process->p_semAdd)++;

			outBlocked(process);
		}
		/* [Case 2] Process blocked on the pseudo-clock semaphore */
		if (process->p_isBlocked)
			/* Performs a V on the semaphore */
			PseudoClock++;
	}
	
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
@brief (SYS3) Perform a V operation on a semaphore.
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

		/* Update p_isBlocked flag */
		process->p_isBlocked = FALSE;
	}
}

/*
@brief (SYS4) Perform a P operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
EXTERN void passeren(int *semaddr)
{
	/* Perform a P on the semaphore */
	(*semaddr)--;

	/* If semaphore value becomes negative */
	if ((*semaddr) < 0)
	{
		/* Block process into the semaphore queue */
		if (insertBlocked(semaddr, CurrentProcess))
			PANIC(); /* Anomaly */

		CurrentProcess->p_isBlocked = TRUE;
		CurrentProcess = NULL;
	}


}

/*
@brief Specify Exception State Vector.
@param type Type of exception.
@param stateOld The address into which the old processor state is to be stored when an exception 
occurs while running this process.
@param stateNew The processor state area that is to be taken as the new processor state if an
exception occurs while running this process.
@return Void.
*/
EXTERN void specTrapVec(int type, state_t *stateOld, state_t *stateNew)
{
	/* Exception types range between 0 and 2 */
	if (type < 0 || type > 2)
	{
		/* Terminate the process */
		terminateProcess();

		/* Anomaly */
		if (CurrentProcess)
			CurrentProcess->p_s.a1 = -1;

		/* Call scheduler */
		scheduler();
	}
	else
	{
		/* Increase the Exception State Vector */
		CurrentProcess->exceptionState[type]++;
		
		/* [Case 1] SYS5 has been called for the first time */
		if (CurrentProcess->exceptionState[type] > 1)
		{
			/* Save the contents of a2 and a3 to facilitate “passing up” 
				handling of the respective exception */
			CurrentProcess->p_stateOldArea[type] = stateOld;
			CurrentProcess->p_stateNewArea[type] = stateNew;
		}
		/* [Case 2] SYS5 has been called for the second time */
		else
		{
			/* Terminate the process */
			terminateProcess();

			/* Anomaly */
			if (CurrentProcess)
				CurrentProcess->p_s.a1 = -1;

			/* Call scheduler */
			scheduler();
		}
	}
}

/*
@brief (SYS6) Retrieve the CPU time of the current process.
@return CPU time of the current process.
*/
EXTERN cpu_t getCPUTime()
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
	termreg_t *terminal;
	dtpreg_t *device;

	switch (interruptLine)
	{
		case INT_DISK:
			passerenIO(&Semaphore.disk[deviceNumber]);
			break;
		case INT_TAPE:
			passerenIO(&Semaphore.tape[deviceNumber]);
			break;
		case INT_UNUSED:
			passerenIO(&Semaphore.network[deviceNumber]);
			break;
		case INT_PRINTER:
			passerenIO(&Semaphore.printer[deviceNumber]);
			break;
		case INT_TERMINAL:
			if (reading)
				passerenIO(&Semaphore.terminalR[deviceNumber]);
			else
				passerenIO(&Semaphore.terminalT[deviceNumber]);
			break;
		default:
			PANIC(); /* Anomaly */
	}

	/* [Case 1] The device is not the terminal */
	if (interruptLine != INT_TERMINAL)
	{
		device = (dtpreg_t *) DEV_REG_ADDR(interruptLine, deviceNumber);
		return device->status;
	}
	/* [Case 2] The device is the terminal */
	else
	{
		terminal = (termreg_t *) DEV_REG_ADDR(interruptLine, deviceNumber);

		/* Distinguish between receiving and transmitting status */
		return (reading)? terminal->recv_status : terminal->transm_status;
	}

	PANIC(); /* Anomaly */
	return 0;
}

/*
@brief TLB (Translation Look Aside Buffer) Exception Handling.
@return void.
*/
EXTERN void tlbHandler()
{
	int ris;

	/* If a process is running */
	if (CurrentProcess)
		/* Save TLB old area into the current process state */
		saveCurrentState(TLB_old, &(CurrentProcess->p_s));
	else
		/* Call scheduler */
		scheduler();

	/* If SYS5 has not been called */
	if (CurrentProcess->exceptionState[TLB_EXCEPTION] == 0)
	{
		/* Terminate the process */
		terminateProcess();

		/* Anomaly */
		if (CurrentProcess)
			CurrentProcess->p_s.a1 = -1;

		/* Call scheduler */
		scheduler();
	}
	else
	{
		/* Save TLB Old Area into the current process state */
		saveCurrentState(TLB_old, CurrentProcess->p_stateOldArea[TLB_EXCEPTION]);
		LDST(CurrentProcess->p_stateNewArea[TLB_EXCEPTION]);
	}
}

/*
@brief Program Trap handler.
@return Void.
*/
EXTERN void pgmTrapHandler()
{
	/* If a process is executing, load pgmTrap Old Area on the current process */
	if (CurrentProcess)
		saveCurrentState(PgmTrap_old, &(CurrentProcess->p_s));

	/* If the offending process has not issued a SYS5, then the PgmTrap exception
	 * should be handled like a SYS2. */
	if (CurrentProcess->exceptionState[PROGRAM_TRAP_EXCEPTION] == 0)
	{
		/* Terminate the process */
		terminateProcess();

		/* Anomaly */
		if (CurrentProcess)
			CurrentProcess->p_s.a1 = -1;

		/* Call the scheduler */
		scheduler();
	}
	/* Pass up the handling of the PgmTrap */
	else
	{
		saveCurrentState(PgmTrap_old, CurrentProcess->p_stateOldArea[PROGRAM_TRAP_EXCEPTION]);
		LDST(CurrentProcess->p_stateNewArea[PROGRAM_TRAP_EXCEPTION]);
	}
}
