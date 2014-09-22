/*
@file exceptions.c
@note SYS/Bp Exception Handling
*/

#include "../e/inclusions.e"

/* Old Area delle Syscall/BP - TLB - Program Trap */
HIDDEN state_t *SysBP_old = (state_t *) SYSBK_OLDAREA;
HIDDEN state_t *TLB_old = (state_t *) TLB_OLDAREA;
HIDDEN state_t *PgmTrap_old = (state_t *) PgmTrap_oldAREA;

/**
@brief Save current state in a new state.
@param currentState Current state.
@param newState New state.
@return Void.
*/
void saveCurrentState(state_t *currentState, state_t *newState)
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
@brief Performs a P on a device semaphore and increments the soft-block count, i.e. increase 
by one the number of processes in the system currently blocked and waiting for an interrupt .
@param *semaddr Semaphore's address
@return Void.
*/
HIDDEN void passerenIO(int *semaddr)
{
	/* Perform a P on the semaphore */
	if (--(*semaddr) < 0)
	{
		/* Block current process in the semaphore queue */
		if (insertBlocked(semaddr, currentProcess))
			PANIC(); /* Anomaly */

		currentProcess->p_isOnSem = TRUE
		currentProcess = NULL;
		softBlockCount++;

		/* Call the scheduler */
		scheduler();
	}
}

/**
@brief This function handles SYSCALL or Breakpoint exceptions, which occurs when a SYSCALL or BREAK assembler
instruction is executed.
@return Void.
*/
void sysBpHandler()
{
	int exceptionType, statusMode;

	/* Save SysBP Old Area state */
	saveCurrentState(SysBP_old, &(currentProcess->p_s));

	/* Avoid endless loop of system calls */
	currentProcess->p_s.pc += 4;

	/* Get status mode of the SysBP Old Area */
	statusMode = SysBP_old->cpsr & STATUS_SYS_MODE;

	/* Get exception type */
	exceptionType = CAUSE_EXCCODE_GET(SysBP_old->CP15_Cause);

	/* [Case 1] It's a system call */
	if (exceptionType == EXC_SYSCALL)
	{
		/* [Case 1.1] User Mode is on */
		if (statusMode == STATUS_USER_MODE)
		{
			/* [Case 1.1.1] One of the available system calls has been called */
			if (SysBP_old->a1 > 0 && SysBP_old->a1 <= 8)
			{
				/* Save SYS/BP Old Area into PgmTrap Old Area */
				saveCurrentState(SysBP_old, PgmTrap_old);

				/* Set trap cause as Reserved Instruction */
				PgmTrap_old->CP15_Cause = CAUSE_EXCCODE_SET(PgmTrap_old->CP15_Cause, EXC_RESERVEDINSTR);

				/* Call the trap handler */
				pgmTrapHandler();
			}
			/* [Case 1.1.2] No system calls has been called */
			else
			{
				/* [Case 1.1.2.1] SYS5 has not been executed */
				if (currentProcess->exceptionState[SYSBKExc] == 0)
				{
					/* Terminate the current process */
					terminateProcess();

					/* Anomaly */
					if (currentProcess)
						currentProcess->p_s.a1 = -1;

					/* Call the scheduler */
					scheduler();
				}
				/* [Case 1.1.2.1] SYS5 has been executed */
				else
				{
					/* Save SYS/BP Old Area in the current process Old Area*/
					saveCurrentState(SysBP_old, currentProcess->p_state_old[SYSBKExc]);

					/* Load processor state */
					LDST(currentProcess->p_state_new[SYSBKExc]);
				}
			}
		}
		/* [Case 1.2] Kernel Mode on */
		else
		{
			/* Save SYSCALL parameters */
			U32 a2 = SysBP_old->a2;
			U32 a3 = SysBP_old->a3;
			U32 a4 = SysBP_old->a4;

			/* Handle the system call */
			switch (SysBP_old->a1)
			{
				case CREATEPROCESS:
					currentProcess->p_s.a1 = createProcess((state_t *) a2);
					break;
				case TERMINATEPROCESS:
					/* Terminate the current process */
					terminateProcess();

					/* Anomaly */
					if (currentProcess)
						currentProcess->p_s.a1 = -1;

					/* Call the scheduler */
					scheduler();
					break;
				case VERHOGEN:
					verhogen((int *) a2);
					break;
				case PASSEREN:
					passeren((int *) a2);
					break;
				case GETCPUTIME:
					currentProcess->p_s.a1 = getCPUTime();
					break;
				case WAITCLOCK:
					waitClock();
					break;
				case WAITIO:
					currentProcess->p_s.a1 = waitIO((int) a2, (int) a3, (int) a4);
					break;
				case SPECTRAPVEC:
					specTrapVec((int) a2, (state_t *) a3, (state_t *) a4);
					break;
				default:
					/* If SYS5 has not been yet executed, the current process shall terminate */
					if (currentProcess->exceptionState[SYSBKExc] == 0)
					{
						/* Terminate the current process */
						terminateProcess();

						/* Anomaly */
						if (currentProcess)
							currentProcess->p_s.a1 = -1;

						/* Call the scheduler */
						scheduler();
					}
					/* Otherwise save SYS/BP old area in the current process */
					else
					{
						saveCurrentState(SysBP_old, currentProcess->p_state_old[SYSBKExc]);
						LDST(currentProcess->p_state_new[SYSBKExc]);
					}
			}

			/* Call the scheduler */
			scheduler();
		}
	}
	/* [Case 2] It's a breakpoint */
	else if (exceptionType == EXC_BREAKPOINT)
	{
		/* If SYS5 has not been yet executed, the current process shall terminate */
		if (currentProcess->exceptionState[SYSBKExc] == 0)
		{
			/* Terminate the current process */
			terminateProcess();

			/* Anomaly */
			if (currentProcess)
				currentProcess->p_s.a1 = -1;

			/* Call the scheduler */
			scheduler();
		}
		else
		{
			/* Save SysBP Old Area in the current process */
			saveCurrentState(sysBk_old, currentProcess->p_state_old[SYSBKExc]);

			/* Load the processor state */
			LDST(currentProcess->p_state_new[SYSBKExc]);
		}
	}
	/* Anomaly */
	else
		PANIC();
}

/**
@brief (SYS1) Create a new process.
@param statep Processor state from which create a new process.
@return -1 in case of failure; PID in case of success.
*/
int createProcess(state_t *statep)
{
	int i;
	pcb_t *p;

	/* Allocate a new PCB */
	if (!(p = allocPcb()))
		return -1;

	/* Load processor state into process state */
	saveCurrentState(statep, &(p->p_s));

	/* Update process and PID counter */
	processCount++;
	pidCount++;
	p->p_pid = pidCount;

	/* Search an empy cell in pcbused_table */
	for (i = 0; i < MAXPROC && pcbused_table[i].pid != 0; i++);

	/* Update pcbused_table */
	pcbused_table[i].pid = p->p_pid;
	pcbused_table[i].pcb = p;

	/* Update process tree and process queue */
	insertChild(currentProcess, p);
	insertProcQ(&readyQueue, p);

	return pidCount;
}

/**
@brief (SYS2) Terminate a process and all its progeny. In case a process in blocked on a semaphore, performs a V on it.
@return 0 in case of success; -1 in case of error.
*/
HIDDEN void terminateProcess()
{
	/* There is not a running process */
	if (!currentProcess) return;

	/* Make the current process block no longer the child of its parent. */
	if (!outChild(currentProcess))
		return -1; /* Anomaly */

	/* Call recursive function */
	_terminateProcess(currentProcess);

	currentProcess = NULL;
}

/**
Recursively terminate a process and its progeny.
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
		if (process->p_isOnSem)
		{
			/* Performs a V on the semaphore */
			(*process->p_semAdd)++;

			outBlocked(process);
		}
		/* [Case 2] Process blocked on the pseudo-clock semaphore */
		if (process->p_isOnSem)
			/* Performs a V on the semaphore */
			pseudo_clock++;
	}
	
	processCount--;
	
	/* Insert the process block into the pcbFree list */
 	freePcb(process);
}

/**
@brief (SYS3) Perform a V operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
void verhogen(int *semaddr)
{
	pcb_t *process;

	/* Perform a V on the semaphore */
	(*semaddr)++;

	/* If ASL is not empty */
	if ((process = removeBlocked(semaddr)))
	{
		/* Insert process into the ready queue */
		insertProcQ(&readyQueue, process);

		/* Update p_isOnSem flag */
		process->p_isOnSem = FALSE;
	}
}

/**
@brief (SYS4) Perform a P operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
void passeren(int *semaddr)
{
	/* Perform a P on the semaphore */
	(*semaddr)--;

	/* If semaphore value becomes negative */
	if ((*semaddr) < 0)
	{
		/* Block process into the semaphore queue */
		if (insertBlocked(semaddr, currentProcess))
			PANIC(); /* Anomaly */

		currentProcess->p_isOnSem = TRUE;
		currentProcess = NULL;
	}


}

/**
@brief Specify Exception State Vector.
@param type Type of exception.
@param stateOld The address into which the old processor state is to be stored when an exception 
occurs while running this process.
@param stateNew The processor state area that is to be taken as the new processor state if an
exception occurs while running this process.
@return Void.
*/
void specTrapVec(int type, state_t *stateOld, state_t *stateNew)
{
	/* Exception types range between 0 and 2 */
	if (type < 0 || type > 2)
	{
		/* Terminate the process */
		terminateProcess();

		/* Anomaly */
		if (currentProcess)
			currentProcess->p_s.a1 = -1;

		/* Call scheduler */
		scheduler();
	}
	else
	{
		/* Increase the Exception State Vector */
		currentProcess->exceptionState[type]++;
		
		/* [Case 1] SYS5 has been called for the first time */
		if (currentProcess->exceptionState[type] > 1)
		{
			/* Save the contents of a2 and a3 to facilitate “passing up” 
				handling of the respective exception */
			currentProcess->p_state_old[type] = stateOld;
			currentProcess->p_state_new[type] = stateNew;
		}
		/* [Case 2] SYS5 has been called for the second time */
		else
		{
			/* Terminate the process */
			terminateProcess();

			/* Anomaly */
			if (currentProcess)
				currentProcess->p_s.a1 = -1;

			/* Call scheduler */
			scheduler();
		}
	}
}

/*
@brief (SYS6) Retrieve the CPU time of the current process.
@return CPU time of the current process.
*/
cpu_t getCPUTime()
{
	/* Perform a last update of the CPU time */
	currentProcess->p_cpu_time += getTODLO() - processTOD;
	processTOD = getTODLO();

	return currentProcess->p_cpu_time;
}

/**
  * @brief (SYS7) Effettua una P sul semaforo dello pseudo-clock.
  * @return void.
 */
void waitClock()
{
	/* Just call the passeren method */
	passerenIO(&pseudo_clock);
}

/**
@brief (SYS8) Perform a P operation on a device semaphore.
@param interruptLine Interrupt line.
@param deviceNumber Device number.
@param reading : TRUE in case of reading; FALSE else.
@return Return the device's Status Word.
*/
unsigned int waitIO(int interruptLine, int deviceNumber, int reading)
{
	termreg_t *terminal;
	dtpreg_t *device;

	switch (interruptLine)
	{
		case INT_DISK:
			passerenIO(&sem.disk[deviceNumber]);
			break;
		case INT_TAPE:
			passerenIO(&sem.tape[deviceNumber]);
			break;
		case INT_UNUSED:
			passerenIO(&sem.network[deviceNumber]);
			break;
		case INT_PRINTER:
			passerenIO(&sem.printer[deviceNumber]);
			break;
		case INT_TERMINAL:
			if (reading)
				passerenIO(&sem.terminalR[deviceNumber]);
			else
				passerenIO(&sem.terminalT[deviceNumber]);
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
@brief TLB (Translation Lookaside Buffer) Exception Handling.
@return void.
*/
void tlbHandler()
{
	int ris;

	/* If a process is running */
	if (currentProcess)
		/* Save TLB old area into the current process state */
		saveCurrentState(TLB_old, &(currentProcess->p_s));
	else
		/* Call scheduler */
		scheduler();

	/* If SYS5 has not been called */
	if (currentProcess->exceptionState[TLBExc] == 0)
	{
		/* Terminate the process */
		terminateProcess();

		/* Anomaly */
		if (currentProcess)
			currentProcess->p_s.a1 = -1;

		/* Call scheduler */
		scheduler();
	}
	else
	{
		/* Save TLB Old Area into the current process state */
		saveCurrentState(TLB_old, currentProcess->tlbState_old);
		LDST(currentProcess->tlbState_new);
	}
}

/**
@brief Program Trap handler.
@return Void.
*/
void pgmTrapHandler()
{
	int ris;

	/* If a process is executing, load pgmTrap Old Area on the current process */
	if (currentProcess)
		saveCurrentState(PgmTrap_old, &(currentProcess->p_s));

	/* If the offending process has NOT issued a SYS5, then the PgmTrap exception
	 * should be handled like a SYS2. */
	if (currentProcess->ExStVec[ESV_PGMTRAP] == 0)
	{
		/* Terminate the process */
		terminateProcess();

		/* Anomaly */
		if (currentProcess)
			currentProcess->p_s.a1 = -1;

		/* Call the scheduler */
		scheduler();
	}
	/* Pass up the handling of the PgmTrap */
	else
	{
		saveCurrentState(PgmTrap_old, currentProcess->pgmtrapState_old);
		LDST(currentProcess->pgmtrapState_new);
	}
}
