/**
@file interrupts.c
@author
@note Interrupt Exception Handling.
*/

#include "../../phase1/h/asl.h"
#include "../../phase1/e/pcb.e"
#include "../e/exceptions.e"
#include "../h/initial.h"
#include "../e/interrupts.e"
#include "../e/scheduler.e"
#include "../../include/libuarm.h"
#include "../../include/const.h"
#include "../../include/arch.h"

/* Old Area dell'Interrupt */
HIDDEN state_t *int_old_area = (state_t *) INT_OLDAREA;

/* Indirizzo base del device */
HIDDEN memaddr devAddrBase;

/* Variabili per accedere ai campi status e command di device o terminali */
HIDDEN int *rStatus, *tStatus;
HIDDEN int *rCommand, *tCommand;
HIDDEN int recvStatByte, transmStatByte;
HIDDEN int *status, *command;

HIDDEN int recognizeDev(int bitMapDevice)
{
	switch (bitMapDevice)
	{
	case DEV_CHECK_ADDRESS_0:
		return DEV_CHECK_LINE_0;
	case DEV_CHECK_ADDRESS_1:
		return DEV_CHECK_LINE_1;
	case DEV_CHECK_ADDRESS_2:
		return DEV_CHECK_LINE_2;
	case DEV_CHECK_ADDRESS_3:
		return DEV_CHECK_LINE_3;
	case DEV_CHECK_ADDRESS_4:
		return DEV_CHECK_LINE_4;
	case DEV_CHECK_ADDRESS_5:
		return DEV_CHECK_LINE_5;
	case DEV_CHECK_ADDRESS_6:
		return DEV_CHECK_LINE_6;
	default:
		return DEV_CHECK_LINE_7;
	}
}

HIDDEN pcb_t *verhogenInt(int *semaddr, int status, int line, int dev)
{
	pcb_t *process;
	
	(*semaddr)++;
	
	/* Se non sono stati sbloccati dei processi, ritorna lo status Word del device */
	if (!(process = removeBlocked(semaddr)))
		statusWordDev[line][dev] = status;
	else
	{
		insertProcQ(&readyQueue, process);
		process->p_isOnDev = FALSE;
		softBlockCount--;
		process->p_s.v1 = status;
	}
	
	return process;
}

HIDDEN void intTimer()
{
	pcb_t *process;

	/* tprint("intTimer...\n"); */
	/* Aggiorna il tempo trascorso */
	timerTick += (getTODLO() - startTimerTick);
	startTimerTick = getTODLO();

	/* Se è arrivato l'interrupt dallo pseudo-clock */
	if (timerTick >= SCHED_PSEUDO_CLOCK)
	{
		/* Se sono state fatte più SYS7 precedentemente */
		if (pseudo_clock < 0)
		{
			/* Sblocca tutti i processi bloccati */
			while (pseudo_clock < 0)
			{
				process = removeBlocked(&pseudo_clock);
				/* Se sono stati sbloccati dei processi ... */
				if (process)
				{ 	/* Se pseudo_clock < 0 deve esserci almeno un processo bloccato */
					insertProcQ(&readyQueue, process);
					process->p_isOnDev = FALSE;
					softBlockCount--;
				}
				pseudo_clock++;
			}
		}
		else
		{
			process = removeBlocked(&pseudo_clock);
			/* Se non viene sbloccato nessun processo (pseudo-V), decrementa lo pseudo-clock */
			if (!process)
				pseudo_clock--;
			/* Altrimenti esegue la V sullo pseudo-clock */
			else
			{
				insertProcQ(&readyQueue, process);
				process->p_isOnDev = FALSE;
				softBlockCount--;
				pseudo_clock++;
			}
		}

		/* Riavvia il tempo per il calcolo dello pseudo-clock tick */
		timerTick = 0;
		startTimerTick = getTODLO();
	}
	/* Se è finito il timeslice del processo corrente */
	else if (currentProcess)
	{
		/* Reinserisce il processo nella Ready Queue */
		insertProcQ(&readyQueue, currentProcess);

		currentProcess = NULL;
		softBlockCount++;

		/* Aggiorna il tempo trascorso */
		timerTick += (getTODLO() - startTimerTick);
		startTimerTick = getTODLO();
	}
	/* Altre cause */
	else
		setTIMER(SCHED_PSEUDO_CLOCK - timerTick);
}

HIDDEN void devInterrupt(int cause)
{
	int *bitMapDevice;
	int devNumb;
	pcb_t *process;

	/* tprint("devInterrupt...\n"); */
	/* Dall'indirizzo iniziale delle bitmap degli interrupt pendenti si accede a quella corrispondente alla propria linea */
	bitMapDevice =(int *) (PENDING_BITMAP_START + (WORD_SIZE * (cause - DEV_DIFF)));

	/* Cerca all'interno della bitmap di questa specifica linea qual è il device con priorità più alta con un interrupt pendente */
	devNumb = recognizeDev(*bitMapDevice);

	/* Cerca l'indirizzo del device */
	devAddrBase = (memaddr) (DEV_IL_START  + ((cause - DEV_DIFF) * CHECK_EIGHTH_BIT) + (devNumb * CHECK_FIFTH_BIT));

	/* Recupera il campo status del device */
	status = (int *) (devAddrBase + DEV_REG_STATUS);
	/* Recupera il campo command del device */
	command = (int *) (devAddrBase + DEV_REG_COMMAND);

	process = verhogenInt(&sem.disk[devNumb], (*status), cause - DEV_DIFF, devNumb);

	/* ACK per il riconoscimento dell'interrupt pendente */
	(*command) = DEV_C_ACK;
}

HIDDEN void intTerminal()
{
	int *bitMapDevice;
	int devNumb;
	pcb_t *process;

	/* tprint("intTerminal...\n"); */

	/* Dall'indirizzo iniziale delle bitmap degli interrupt pendenti si accede a quella corrispondente alla propria linea */
	bitMapDevice =(int *) (PENDING_BITMAP_START + (WORD_SIZE * (INT_TERMINAL - DEV_DIFF)));

	/* Cerca all'interno della bitmap di questa specifica linea qual è il device con priorità più alta con un interrupt pendente */
	devNumb = recognizeDev(*bitMapDevice);

	/* Cerca l'indirizzo del device */
	devAddrBase = (memaddr) (DEV_IL_START  + ((INT_TERMINAL - DEV_DIFF) * CHECK_EIGHTH_BIT) + (devNumb * CHECK_FIFTH_BIT));

	/* Device status field (receiving) */
	rStatus	= (int *) (devAddrBase + TERM_R_STATUS);
	/* Device status field (sending) */
	tStatus	= (int *) (devAddrBase + TERM_T_STATUS);
	/* Device command field (receiving) */
	rCommand = (int *) (devAddrBase + TERM_R_COMMAND);
	/* Device command field (sending) */
	tCommand = (int *) (devAddrBase + TERM_T_COMMAND);

	/* Extract status byte to check what happened */
	recvStatByte   = (*rStatus) & CHECK_STATUS_BIT;
	transmStatByte = (*tStatus) & CHECK_STATUS_BIT;

	/* [Case 1] Sent character */
	if (transmStatByte == DEV_TTRS_S_CHARTRSM)
	{
		/* tprint("Sent character...\n"); */
		/* Perform a V on the device semaphore */
		process = verhogenInt(&sem.terminalT[devNumb], (*tStatus), INT_TERMINAL - DEV_DIFF, devNumb);
		/* ACK per il riconoscimento dell'interrupt pendente */
		(*tCommand) = DEV_C_ACK;
	}

	/* [Case 2] Received character */
	else if (recvStatByte == DEV_TRCV_S_CHARRECV)
	{
		/* tprint("Received character...\n"); */
		/* Perform a V on the device semaphore */
		process = verhogenInt(&sem.terminalR[devNumb], (*rStatus), INT_TERMINAL - DEV_DIFF + 1, devNumb);
		/* ACK per il riconoscimento dell'interrupt pendente */
		(*rCommand) = DEV_C_ACK;
	}
}

void intHandler()
{
	int cause_int;
	
	/* If a process is running, load the Interrupt Old Area */
	if (currentProcess)
		saveCurrentState(int_old_area , &(currentProcess->p_s));
	
	/* Get interrupt cause */
	cause_int = int_old_area->CP15_Cause;
	
	if (CAUSE_IP_GET(cause_int, INT_TIMER))
		intTimer();
	else if (CAUSE_IP_GET(cause_int, INT_DISK))
		devInterrupt(INT_DISK);
	else if (CAUSE_IP_GET(cause_int, INT_TAPE))
		devInterrupt(INT_TAPE);
	else if (CAUSE_IP_GET(cause_int, INT_UNUSED))
		devInterrupt(INT_UNUSED);
	else if (CAUSE_IP_GET(cause_int, INT_PRINTER))
		devInterrupt(INT_PRINTER);
	else if (CAUSE_IP_GET(cause_int, INT_TERMINAL))
		intTerminal();

	scheduler();
}
