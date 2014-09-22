/**
 *  @file exceptions.c
 *  @author
 *  @note SYS/Bp Exception Handling
 */

#include "../../phase1/h/asl.h"
#include "../../phase1/e/pcb.e"
#include "../e/exceptions.e"
#include "../h/initial.h"
#include "../e/interrupts.e"
#include "../e/scheduler.e"
#include "../../include/libuarm.h"

/* Old Area delle Syscall/BP - TLB - Program Trap */
HIDDEN state_t *sysBp_old = (state_t *) SYSBK_OLDAREA;
HIDDEN state_t *TLB_old = (state_t *) TLB_OLDAREA;
HIDDEN state_t *pgmTrap_old = (state_t *) PGMTRAP_OLDAREA;

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
@brief Esegue una 'P' sul semaforo passato per parametro. E' diversa dalla SYS4 siccome viene incrementato il softBlockCount.
@param *semaddr : puntatore al semaforo su cui fare la 'P'
@return void.
*/
HIDDEN void passerenIO(int *semaddr)
{
	if (--(*semaddr) < 0)
	{
		/* Block current process in the semaphore queue */
		if (insertBlocked(semaddr, currentProcess))
			PANIC(); /* Anomaly */


		currentProcess->p_isOnSem = IS_ON_DEV;
		currentProcess = NULL;
		softBlockCount++;

		/* Run scheduler */
		scheduler();
	}
}

/**
@brief SYSCALL/BP handler.
@return Void.
*/
void sysBpHandler()
{
	int exceptionType, statusMode;

	/* Save sysBp old area state */
	saveCurrentState(sysBp_old, &(currentProcess->p_s));

	/* Avoid endless loop of system calls */
	currentProcess->p_s.pc += 4;

	/* Get status mode of the sysBp old area */
	statusMode = sysBp_old->cpsr & STATUS_SYS_MODE;

	/* Get exception type */
	exceptionType = setCAUSE(sysBp_old->CP15_Cause);

	/* [Case 1] It's a system call */
	if (exceptionType == EXC_SYSCALL)
	{
		/* [Case 1.1] User Mode */
		if (statusMode == STATUS_USER_MODE)
		{
			/* [Case 1.1.1] One of the available system calls has been called */
			if (sysBp_old->a1 > 0 && sysBp_old->a1 <= SYSCALL_MAX)
			{
				/* Save SYS/BP old area into pgmTrap old area */
				saveCurrentState(sysBp_old, pgmTrap_old);

				/* Set trap cause as Reserved Instruction */
				pgmTrap_old->CP15_Cause = CAUSE_EXCCODE_SET(pgmTrap_old->CP15_Cause, EXC_RESERVEDINSTR);

				/* Call the trap handler */
				pgmTrapHandler();
			}
			/* [Case 1.1.2] No system calls has been called */
			else
			{
				/* [Case 1.1.2.1] SYS5 has not been executed */
				if (currentProcess->exceptionState[SYSBKExc] == 0)
				{
					/* The current process must terminate */
					int ris;

					ris = terminateProcess(-1);
					if (currentProcess)
						currentProcess->p_s.a1 = ris;
					scheduler();
				}
				/* [Case 1.1.2.1] SYS5 has been executed */
				else
				{
					/* Save SYS/BP old area in the current process */
					saveCurrentState(sysBp_old, currentProcess->p_state_old[SYSBKExc]);
					LDST(currentProcess->p_state_new[SYSBKExc]);
				}
			}
		}
		/* [Case 1.2] Kernel Mode */
		else
		{
			int ris;

			/* Save SYSCALL parameters */
			U32 a2 = sysBp_old->a2;
			U32 a3 = sysBp_old->a3;
			U32 a4 = sysBp_old->a4;

			/* Handle the system call */
			switch (sysBp_old->a1)
			{
				case CREATEPROCESS:
					currentProcess->p_s.a1 = createProcess((state_t *) a2);
					break;
				case TERMINATEPROCESS:
					ris = terminateProcess((int) a2);
					if (currentProcess)
						currentProcess->p_s.a1 = ris;
					break;
				case VERHOGEN:

					verhogen((int *) a2);
					break;
				case PASSEREN:
					passeren((int *) a2);
					break;
				case GETPID:
					currentProcess->p_s.a1 = getPid();
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
				case GETPPID:
					currentProcess->p_s.a1 = getPpid();
					break;
				case SPECTRAPVEC:
					specTrapVec((int) a2, (state_t *) a3, (state_t *) a4);
					break;
				default:
					/* If SYS12 has not been yet executed, the current process shall terminate */
					if (currentProcess->ExStVec[ESV_SYSBP] == 0)
					{
						int ris;

						ris = terminateProcess(-1);
						if (currentProcess)
							currentProcess->p_s.a1 = ris;
					}
					/* Otherwise save SYS/BP old area in the current process */
					else
					{
						saveCurrentState(sysBp_old, currentProcess->p_state_old[SYSBKExc]);
						LDST(currentProcess->p_state_new[SYSBKExc]);
					}
			}

			/* Call the scheduler */
			currentProcess->p_s.pc -= 4;
			scheduler();
		}
	}
	/* [Case 2] It's a breakpoint */
	else if (exceptionType == EXC_BREAKPOINT)
	{
		/* If SYS12 has not been yet executed, the current process shall terminate */
		if (currentProcess->exceptionState[SYSBKExc] == 0)
		{
			int ris;

			ris = terminateProcess(-1);
			if (currentProcess)
				currentProcess->p_s.v1 = ris;
			scheduler();
		}
		/* Otherwise save SysBP Old Area in the current process */
		else
		{
			saveCurrentState(sysBk_old, currentProcess->p_state_old[SYSBKExc]);
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
@brief (SYS2) Terminate a process and all his progeny.
@param pid Process Id.
@return 0 in case of success; -1 in case of error.
*/
int terminateProcess(int pid)
{
	int i, selfTermination;
	pcb_t *process, *child, *pSib;

	process = NULL;
	selfTermination = FALSE;

	/* Self-termination case */
	if (pid == -1 || currentProcess->p_pid == pid)
	{
		pid = currentProcess->p_pid;
		selfTermination = TRUE;
	}

	/* Search process in pcbused_table */
	for (i = 0; i < MAXPROC && pid != pcbused_table[i].pid; i++);

	/* If process does not exist */
	if (i == MAXPROC)
		return -1; /* Anomaly */

	process = pcbused_table[i].pcb;

	/* [Case 1] Process blocked on a device semaphore */
	if (process->p_isOnSem == IS_ON_SEM)
	{
		if (!process->p_semAdd)
			PANIC(); /* Anomaly */

		/* Update semaphore value */
		(*process->p_semAdd)++;
		process = outBlocked(process);
	}
	/* [Case 2] Process blocked on the pseudo-clock semaphore */
	else if (process->p_isOnSem == IS_ON_PSEUDO)
		pseudo_clock++;

	/* If process has a progeny */
	while (!emptyChild(process))
	{
		child = removeChild(process);

		if ((terminateProcess(child->p_pid)) == -1)
			return -1; /* Anomaly */
	}

	/* Kill process */
	if (!outChild(process))
		return -1; /* Anomaly */

	/* Update pcbused_table */
	pcbused_table[i].pid = 0;
	pcbused_table[i].pcb = NULL;

	freePcb(process);

	if (selfTermination)
		currentProcess = NULL;

	processCount--;
	return 0;
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
 *	specTrapVec - save the oldp and the newp in the Current Process to make easier the passed up
 *	@type: 0 for TLB exceptions; 1 for PgmTrap exceptions; 2 for SYS/Bp exceptions;
 *	@oldp: address that store the old processor state.
 *	@newp: address of the processor state area that is to be taken as the new processor state if an
 *		exception occurs while running this process.
 *
 *	Specify Exception State Vector
 *	When this service is requested, the nucleus will save the contents of a3 and
 *	a4 (in the invoking processes' ProcBlk) to facilitate “passing up” handling of the
 *	respective exception when (and if) one occurs while this process is executing.
 *	When an exception occurs for which an Exception State Vector has been specified
 *	for, the nucleus stores the processor state at the time of the exception in the area
 *	pointed to by the address in a3, and loads the new processor state from the area
 *	pointed to by the address given in a4.
 *	Each process may request a SYS5 service at most once for each of three exception types.
 *	An attempt to request a SYS5 service more than once per exception type by any process
 *	should be considered as an error and treated as a SYS2.
 */
void specTrapVec(int type, state_t *oldp, state_t *newp)
{
	/* if type is not consistent */
	if ((type < 0) || (type > 2)) {

		terminateProcess();

		if (currentProcess != NULL)
			currentProcess->p_s.a1 = -1;

	}
	else {

		/* we increase exceptionState so we know sys 5 was called */
		currentProcess->exceptionState[type]++;

		if (currentProcess->exceptionState[type] > 1) {	/* case: SYS5 called more then once, then SYS2 */

			terminateProcess();

			if (currentProcess != NULL)
				currentProcess->p_s.a1 = -1;

		}
		else {

			currentProcess->p_state_old[type] = oldp;
			currentProcess->p_state_new[type] = newp;

		}
	}
}

/**
@brief (SYS5) Restituisce l'identificativo del processo chiamante.
@return PID del processo chiamante.
*/
int getPid()
{
	return currentProcess->p_pid;
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
		ris = terminateProcess(-1);
		if (currentProcess)
			currentProcess->p_s.v1 = ris;

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
		saveCurrentState(pgmTrap_old, &(currentProcess->p_s));

	/* If the offending process has NOT issued a SYS5, then the PgmTrap exception
	 * should be handled like a SYS2. */
	if (currentProcess->ExStVec[ESV_PGMTRAP] == 0)
	{
		ris = terminateProcess(-1);
		/* Anomaly */
		if (currentProcess)
			currentProcess->p_s.v1 = ris;
		scheduler();
	}
	/* Pass up the handling of the PgmTrap */
	else
	{
		saveCurrentState(pgmTrap_old, currentProcess->pgmtrapState_old);
		LDST(currentProcess->pgmtrapState_new);
	}
}
