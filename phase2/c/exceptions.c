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
@param current Current state.
@param new New state.
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
	(*semaddr)--;

	if((*semaddr) < 0)
	{
		/* Inserisce il processo corrente in coda al semaforo specificato */
		if(insertBlocked((S32 *) semaddr, currentProcess)) PANIC(); /* PANIC se sono finiti i descrittori dei semafori */
		currentProcess->p_isOnDev = IS_ON_DEV;
		currentProcess = NULL;
		softBlockCount++;
		scheduler();
	}
}

/**
@brief SYSCALL/BP handler.
@return Void.
*/
void sysBpHandler()
{
	int cause_excCode;
	int kuMode;

	/* Save SYSBK_OLDAREA old state */
	saveCurrentState(sysBp_old, &(currentProcess->p_s));

	/* Avoid endless loop of system calls */
	currentProcess->p_s.pc += 4;

	/* Get Mode bit of the sysBp Old Area */
	kuMode = ((sysBp_old->cpsr) & STATUS_SYS_MODE) >> 0x5;

	/* Get exception type */
	cause_excCode = setCAUSE(sysBp_old->CP15_Cause);

	/* If it is a system call */
	if (cause_excCode == EXC_SYSCALL)
	{
		/* Case User Mode */
		if (kuMode)
		{
			/* One of the 13 system calls has been called */
			if (sysBp_old->a1 > 0 && sysBp_old->a1 <= SYSCALL_MAX)
			{
				/* Imposta Cause.ExcCode a RI
				sysBp_old->cause = CAUSE_EXCCODE_SET(sysBp_old->cause, EXC_RESERVEDINSTR);
				*/
				/* Save SysBP Old Area status pgmTrap Old Area status */
				saveCurrentState(sysBp_old, pgmTrap_old);

				pgmTrapHandler();
			}
			else
			{
				/* If SYS12 has not been executed the current process must terminate */
				if (currentProcess->ExStVec[ESV_SYSBP] == 0)
				{
					int ris;

					ris = terminateProcess(-1);
					if (currentProcess)
						currentProcess->p_s.v1 = ris;
					scheduler();
				}
				/* Save SysBP Old Area in the current process */
				else
				{
					saveCurrentState(sysBp_old, currentProcess->sysbpState_old);
					LDST(currentProcess->sysbpState_new);
				}
			}
		}
		/* Case Kernel Mode */
		else
		{
			int ris;

			/* Save SYSCALL parameters */
			U32 arg1 = sysBp_old->a2;
			U32 arg2 = sysBp_old->a3;
			U32 arg3 = sysBp_old->a4;

			/* Handle the system call */
			switch (sysBp_old->a1)
			{
				case CREATEPROCESS:
					currentProcess->p_s.v1 = createProcess((state_t *) arg1);
					break;
				case TERMINATEPROCESS:
					ris = terminateProcess((int) arg1);
					if (currentProcess)
						currentProcess->p_s.v1 = ris;
					break;
				case VERHOGEN:
					verhogen((int *) arg1);
					break;
				case PASSEREN:
					passeren((int *)  arg1);
					break;
				case GETPID:
					currentProcess->p_s.v1 = getPid();
					break;
				case GETCPUTIME:
					currentProcess->p_s.v1 = getCPUTime();
					break;
				case WAITCLOCK:
					waitClock();
					break;
				case WAITIO:
					currentProcess->p_s.v1 = waitIO((int) arg1, (int) arg2, (int) arg3);
					break;
				case GETPPID:
					currentProcess->p_s.v1 = getPpid();
					break;
				case SPECTLBVECT:
					specTLBvect((state_t *) arg1, (state_t *)arg2);
					break;
				case SPECPGMVECT:
					specPGMvect((state_t *) arg1, (state_t *)arg2);
					break;
				case SPECSYSVECT:
					specSYSvect((state_t *) arg1, (state_t *)arg2);
					break;
				default:
					/* If SYS12 has not been yet executed, the current process shall terminate */
					if (currentProcess->ExStVec[ESV_SYSBP] == 0)
					{
						int ris;

						ris = terminateProcess(-1);
						if (currentProcess)
							currentProcess->p_s.v1 = ris;
					}
					/* Otherwise save SysBP Old Area in the current process */
					else
					{
						saveCurrentState(sysBp_old, currentProcess->sysbpState_old);
						LDST(currentProcess->sysbpState_new);
					}
			}

			scheduler();
		}
	}

	/* Case Breakpoint */
	else if (cause_excCode == EXC_BREAKPOINT)
	{
		/* If SYS12 has not been yet executed, the current process shall terminate */
		if (currentProcess->ExStVec[ESV_SYSBP] == 0)
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
			saveCurrentState(sysBp_old, currentProcess->sysbpState_old);
			LDST(currentProcess->sysbpState_new);
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
	int i;
	pcb_t *process, *progeny, *pSib;
	int isSuicide;

	process = NULL;
	isSuicide = FALSE;

	/* Suicide case */
	if (pid == -1 || currentProcess->p_pid == pid)
	{
		pid = currentProcess->p_pid;
		isSuicide = TRUE;
	}

	/* Search process in pcbused_table */
	for (i = 0; i < MAXPROC && pid != pcbused_table[i].pid; i++);

	/* If process does not exist */
	if (i == MAXPROC)
		return -1;

	process = pcbused_table[i].pcb;

	/* [Case 1] Process blocked on a device semaphore */
	if (process->p_isOnDev == IS_ON_SEM)
	{
		if (!process->p_semAdd)
			PANIC(); /* Anomaly */

		/* Update semaphore value */
		(*process->p_semAdd)++;
		process = outBlocked(process);
	}
	/* [Case 2] Process blocked on the pseudo-clock semaphore */
	else if (process->p_isOnDev == IS_ON_PSEUDO)
		pseudo_clock++;

	/* If process has a progeny */
	while (!emptyChild(process))
	{
		progeny = removeChild(process);

		if ((terminateProcess(progeny->p_pid)) == -1)
			return -1; /* Anomaly */
	}

	/* Kill process */
	if (!outChild(process))
		return -1;

	/* Update pcbused_table */
	pcbused_table[i].pid = 0;
	pcbused_table[i].pcb = NULL;

	freePcb(process);

	if (isSuicide == TRUE)
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
	(*semaddr)++;

	/* If ASL is not empty */
	if ((process = removeBlocked(semaddr)))
	{
		/* Insert process into the ready queue */
		insertProcQ(&readyQueue, process);

		/* Update isOnDev flag */
		process->p_isOnDev = FALSE;
	}
}

/**
@brief (SYS4) Perform a P operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
void passeren(int *semaddr)
{
	(*semaddr)--;

	/* If semaphore value becomes negative */
	if ((*semaddr) < 0)
	{
		/* Add process to the semaphore queue */
		if (insertBlocked(semaddr, currentProcess))
			PANIC(); /* Anomaly */
		currentProcess->p_isOnDev = IS_ON_SEM;
		currentProcess = NULL;
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

/**
  * @brief (SYS6) Restituisce il tempo d'uso della CPU da parte del processo chiamante.
  * @return Restituisce il tempo d'uso della CPU.
 */
cpu_t getCPUTime()
{
	/* Aggiorna il tempo di vita del processo sulla CPU */
	currentProcess->p_cpu_time += (getTODLO() - processTOD);
	processTOD = getTODLO();

	return currentProcess->p_cpu_time;
}

/**
  * @brief (SYS7) Effettua una P sul semaforo dello pseudo-clock.
  * @return void.
 */
void waitClock()
{
	pseudo_clock--;

	if(pseudo_clock < 0)
	{
		/* Inserisce il processo corrente in coda al semaforo specificato */
		if(insertBlocked((S32 *) &pseudo_clock, currentProcess)) PANIC(); /* PANIC se sono finiti i descrittori dei semafori */
		currentProcess->p_isOnDev = IS_ON_PSEUDO;
		currentProcess = NULL;
		softBlockCount++;
		scheduler();
	}

}

/**
@brief (SYS8) Perform a P operation on a device semaphore.
@param intlNo Interrupt line.
@param dnum Device number.
@param waitForTermRead : TRUE in case of reading; FALSE else.
@return Return the device's Status Word.
*/
unsigned int waitIO(int intlNo, int dnum, int waitForTermRead)
{
	switch (intlNo)
	{
		case INT_DISK:
			passerenIO(&sem.disk[dnum]);
			break;
		case INT_TAPE:
			passerenIO(&sem.tape[dnum]);
			break;
		case INT_UNUSED:
			passerenIO(&sem.network[dnum]);
			break;
		case INT_PRINTER:
			passerenIO(&sem.printer[dnum]);
			break;
		case INT_TERMINAL:
			if (waitForTermRead)
				passerenIO(&sem.terminalR[dnum]);
			else
				passerenIO(&sem.terminalT[dnum]);
			break;
		default: PANIC();
	}

	return statusWordDev[intlNo - 3 + waitForTermRead][dnum];
}

/**
  * @brief (SYS9) Restituisce l'identificativo del genitore del processo chiamante.
  * @return -1 se il processo chiamante è il processo radice (init), altrimenti il PID del genitore.
 */
int getPpid()
{
	if(currentProcess->p_pid == 1) return -1;
	else return currentProcess->p_prnt->p_pid;
}

/**
  * @brief (SYS10) Quando occorre un'eccezione di tipo TLB entrambi gli stati del processore (oldp e newp) vengono salvati nel processo.
  *		   Caso mai la SYS10 fosse già stata chiamata dallo stesso processo, viene trattata come una SYS2.
  * @param oldp : l'indirizzo del vecchio stato del processore.
  * @param newp : l'indirizzo del nuovo stato del processore.
  * @return void.
 */
void specTLBvect(state_t *oldp, state_t *newp)
{
	int ris;

	currentProcess->ExStVec[ESV_TLB]++;
	/* Se l'eccezione è già stata chiamata dal processo corrente precedentemente, viene terminato */
	if(currentProcess->ExStVec[ESV_TLB] > 1)
	{
		ris = terminateProcess(-1);
		if(currentProcess != NULL) currentProcess->p_s.v1 = ris;
	}
	else
	{
		/* Altrimenti vengono salvati i due stati del processore nel processo corrente */
		currentProcess->tlbState_old = oldp;
		currentProcess->tlbState_new = newp;
	}
}

/**
  * @brief (SYS11) Quando occorre un'eccezione di tipo Program Trap entrambi gli stati del processore (oldp e newp) vengono salvati nel processo.
  *		   Caso mai la SYS11 fosse già stata chiamata dallo stesso processo, viene trattata come una SYS2.
  * @param oldp : l'indirizzo del vecchio stato del processore
  * @param newp : l'indirizzo del nuovo stato del processore
  * @return void.
 */
void specPGMvect(state_t *oldp, state_t *newp)
{
	int ris;

	currentProcess->ExStVec[1]++;
	/* Se l'eccezione è già stata chiamata dal processo corrente, viene terminato */
	if(currentProcess->ExStVec[ESV_PGMTRAP] > 1)
	{
		ris = terminateProcess(-1);
		if(currentProcess != NULL) currentProcess->p_s.v1 = ris;
	}
	else
	{
		/* Altrimenti vengono salvati i due stati del processore nel processo corrente */
		currentProcess->pgmtrapState_old = oldp;
		currentProcess->pgmtrapState_new = newp;
	}
}

/**
  * @brief (SYS12) Quando occorre un'eccezione di tipo Syscall/Breakpoint entrambi gli stati del processore (oldp e newp) vengono salvati nel processo.
  *		   Caso mai la SYS12 fosse già stata chiamata dallo stesso processo, viene trattata come una SYS2.
  * @param oldp : l'indirizzo del vecchio stato del processore
  * @param newp : l'indirizzo del nuovo stato del processore
  * @return void.
 */
void specSYSvect(state_t *oldp, state_t *newp)
{
	int ris;

	currentProcess->ExStVec[ESV_SYSBP]++;
	/* Se l'eccezione è già stata chiamata dal processo corrente, viene terminato */
	if(currentProcess->ExStVec[ESV_SYSBP] > 1)
	{
		ris = terminateProcess(-1);
		if(currentProcess != NULL) currentProcess->p_s.v1 = ris;
	}
	else
	{
		/* Altrimenti vengono salvati i due stati del processore nel processo corrente */
		currentProcess->sysbpState_old = oldp;
		currentProcess->sysbpState_new = newp;
	}
}

/**
  * @brief Gestione d'eccezione TLB.
  * @return void.
 */
void tlbHandler()
{
	int ris;

	/* Se un processo è attualmente eseguito dal processore, la TLB Old Area viene caricata sul processo corrente */
	if(currentProcess != NULL)
		saveCurrentState(TLB_old, &(currentProcess->p_s));

	/* Se non è già stata eseguita la SYS10, viene terminato il processo corrente */
	if(currentProcess->ExStVec[ESV_TLB] == 0)
	{
		ris = terminateProcess(-1);
		if(currentProcess != NULL) currentProcess->p_s.v1 = ris;
		scheduler();
	}
	/* Altrimenti viene salvata la TLB Old Area all'interno del processo corrente */
	else
	{
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
