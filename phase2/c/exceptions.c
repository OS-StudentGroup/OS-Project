/**
 *  @file exceptions.c
 *  @author
 *  @note SYS/Bp Exception Handling
 */

/* Inclusioni phase1 */
#include "../h/exceptions.h"

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
	newState->v3 = currentState->a3;
	newState->v4 = currentState->a4;
	newState->v5 = currentState->a5;
	newState->v6 = currentState->a6;
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
	saveCurrentState(sysBp_old, &(currentProcess->p_state));

	/* Avoid endless loop of system calls */
	currentProcess->p_state.pc += 4;

	/* Get Mode bit of the sysBp Old Area */
	kuMode = ((sysBp_old->cpsr) & STATUS_SYS_MODE) >> 0x3;
	if ((sysBp_old->cpsr << 27) == 0x1F)
		tprint("System Mode");
	else
		tprint("No System Mode");
	HALT();
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
						currentProcess->p_state.v1 = ris;
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
					currentProcess->p_state.v1 = createProcess((state_t *) arg1);
					break;
				case TERMINATEPROCESS:
					ris = terminateProcess((int) arg1);
					if (currentProcess)
						currentProcess->p_state.v1 = ris;
					break;
				case VERHOGEN:
					verhogen((int *) arg1);
					break;
				case PASSEREN:
					passeren((int *)  arg1);
					break;
				case GETPID:
					currentProcess->p_state.v1 = getPid();
					break;
				case GETCPUTIME:
					currentProcess->p_state.v1 = getCPUTime();
					break;
				case WAITCLOCK:
					waitClock();
					break;
				case WAITIO:
					currentProcess->p_state.v1 = waitIO((int) arg1, (int) arg2, (int) arg3);
					break;
				case GETPPID:
					currentProcess->p_state.v1 = getPpid();
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
							currentProcess->p_state.v1 = ris;
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
				currentProcess->p_state.v1 = ris;
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
	saveCurrentState(statep, &(p->p_state));

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
	pcb_t *pToKill, *pChild, *pSib;
	int isSuicide;

	pToKill = NULL;
	isSuicide = FALSE;

	/* Se è un caso di suicidio, reimposta il pid e aggiorna la flag */
	if ((pid == -1) || (currentProcess->p_pid == pid))
	{
		pid = currentProcess->p_pid;
		isSuicide = TRUE;
	}

	/* Cerca nella tabella dei pcb usati quello da rimuovere */
	for(i=0; i<MAXPROC; i++)
	{
		if(pid == pcbused_table[i].pid)
		{
			pToKill = pcbused_table[i].pcb;
			break;
		}
	}

	/* Se si cerca di uccidere un processo che non esiste, restituisce -1 */
	if(pToKill == NULL) return -1;

	/* Se il processo è bloccato su un semaforo esterno, incrementa questo ultimo */
	if(pToKill->p_isOnDev == IS_ON_SEM)
	{
		/* Caso Anomalo */
		if(pToKill->p_semAdd == NULL) PANIC();

		/* Incrementa il semaforo e aggiorna questo ultimo se vuoto */
		(*pToKill->p_semAdd)++;
		pToKill = outBlocked(pToKill);
	}
	/* Se invece è bloccato sul semaforo dello pseudo-clock, si incrementa questo ultimo */
	else if(pToKill->p_isOnDev == IS_ON_PSEUDO) pseudo_clock++;

	/* Se il processo da uccidere ha dei figli, li uccide ricorsivamente */
	while(emptyChild(pToKill) == FALSE)
	{
		pChild = container_of(pToKill->p_child.prev, pcb_t, p_sib);

		if((terminateProcess(pChild->p_pid)) == -1) return -1;
	}

	/* Uccide il processo */
	if((pToKill = outChild(pToKill)) == NULL) return -1;
	else
	{
		/* Aggiorna la tabella dei pcb usati */
		pcbused_table[i].pid = 0;
		pcbused_table[i].pcb = NULL;

		freePcb(pToKill);
	}

	if(isSuicide == TRUE) currentProcess = NULL;

	processCount--;

	return 0;
}

/**
  * @brief (SYS3) Effettua una V su un semaforo.
  * @param semaddr : indirizzo del semaforo.
  * @return void.
 */
void verhogen(int *semaddr)
{
	pcb_t *p;

	(*semaddr)++;

	p = removeBlocked((S32 *) semaddr);
	/* Se è stato sbloccato un processo da un semaforo esterno */
	if (p != NULL)
	{
		/* Viene inserito nella readyQueue e viene aggiornata la flag isOnDev a FALSE */
		insertProcQ(&readyQueue, p);
		p->p_isOnDev = FALSE;
	}
}

/**
  * @brief (SYS4) Effettua una P su un semaforo.
  * @param semaddr : indirizzo del semaforo.
  * @return void.
 */
void passeren(int *semaddr)
{
	(*semaddr)--;

	/* Se un processo viene sospeso ... */
	if((*semaddr) < 0)
	{
		/* Inserisce il processo corrente in coda al semaforo specificato */
		if(insertBlocked((S32 *) semaddr, currentProcess)) PANIC();
		currentProcess->p_isOnDev = IS_ON_SEM;
		currentProcess = NULL;
	}
}

/**
  * @brief (SYS5) Restituisce l'identificativo del processo chiamante.
  * @return PID del processo chiamante.
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
	currentProcess->p_cpu_time += (getTODLO - processTOD);
	processTOD = getTODLO;

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
  * @brief (SYS8) Effettua una P sul semaforo del device specificato.
  * @param intlNo : ennesima linea di interrupt.
  * @param dnum : numero del device.
  * @param waitForTermRead : TRUE se aspetta una lettura da terminale. FALSE altrimenti.
  * @return Restituisce lo Status Word del device specificato.
 */
unsigned int waitIO(int intlNo, int dnum, int waitForTermRead)
{
	switch(intlNo)
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
			if(waitForTermRead)
				passerenIO(&sem.terminalR[dnum]);
			else
				passerenIO(&sem.terminalT[dnum]);
		break;
		default: PANIC();
	}

	return statusWordDev[intlNo - DEV_DIFF + waitForTermRead][dnum];
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
		if(currentProcess != NULL) currentProcess->p_state.reg_v0 = ris;
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
		if(currentProcess != NULL) currentProcess->p_state.reg_v0 = ris;
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
		if(currentProcess != NULL) currentProcess->p_state.reg_v0 = ris;
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
		saveCurrentState(TLB_old, &(currentProcess->p_state));

	/* Se non è già stata eseguita la SYS10, viene terminato il processo corrente */
	if(currentProcess->ExStVec[ESV_TLB] == 0)
	{
		ris = terminateProcess(-1);
		if(currentProcess != NULL) currentProcess->p_state.reg_v0 = ris;
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
  * @brief Gestore d'eccezione Program Trap.
  * @return void.
 */
void pgmTrapHandler()
{
	int ris;

	/* Se un processo è attualmente eseguito dal processore, la pgmTrap Old Area viene caricata sul processo corrente */
	if(currentProcess != NULL)
	{
		saveCurrentState(pgmTrap_old, &(currentProcess->p_state));
	}

	/* Se non è già stata eseguita la SYS11, viene terminato il processo corrente */
	if(currentProcess->ExStVec[ESV_PGMTRAP] == 0)
	{
		ris = terminateProcess(-1);
		if(currentProcess != NULL) currentProcess->p_state.reg_v0 = ris;
		scheduler();
	}
	/* Altrimenti viene salvata la pgmTrap Old Area all'interno del processo corrente */
	else
	{
		saveCurrentState(pgmTrap_old, currentProcess->pgmtrapState_old);
		LDST(currentProcess->pgmtrapState_new);
	}
}
