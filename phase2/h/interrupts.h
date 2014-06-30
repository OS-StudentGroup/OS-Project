#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/**
@file interrupts.h
@author
@note This module implements the device interrupt exception handler. This module will process all the device interrupts, including Interval Timer interrupts, converting device interrupts into V operations on the appropriate semaphores.
*/

#include "../../phase1/h/pcb.h"
#include "exceptions.h"
#include "initial.h"
#include "scheduler.h"
#include "libumps.h"

/* Old Area dell'Interrupt */
HIDDEN state_t *int_old_area = (state_t *) INT_OLDAREA;

/* Indirizzo base del device */
HIDDEN memaddr devAddrBase;

/* Variabili per accedere ai campi status e command di device o terminali */
HIDDEN int *rStatus, *tStatus;
HIDDEN int *rCommand, *tCommand;
HIDDEN int recvStatByte, transmStatByte;
HIDDEN int *status, *command;

/**
  * @brief Riconosce il device che ha un interrupt pendente sulla bitmap passata per parametro.
  * @param bitMapDevice : bitmap degli interrupt pendenti per linea di interrupt.
  * @return Ritorna l'indice del device che ha un interrupt pendente su quella bitmap.
 */
HIDDEN int recognizeDev(int bitMapDevice)
{
	if(bitMapDevice == (bitMapDevice | DEV_CHECK_ADDRESS_0)) return DEV_CHECK_LINE_0;
	else if(bitMapDevice == (bitMapDevice | DEV_CHECK_ADDRESS_1)) return DEV_CHECK_LINE_1;
	else if(bitMapDevice == (bitMapDevice | DEV_CHECK_ADDRESS_2)) return DEV_CHECK_LINE_2;
	else if(bitMapDevice == (bitMapDevice | DEV_CHECK_ADDRESS_3)) return DEV_CHECK_LINE_3;
	else if(bitMapDevice == (bitMapDevice | DEV_CHECK_ADDRESS_4)) return DEV_CHECK_LINE_4;
	else if(bitMapDevice == (bitMapDevice | DEV_CHECK_ADDRESS_5)) return DEV_CHECK_LINE_5;
	else if(bitMapDevice == (bitMapDevice | DEV_CHECK_ADDRESS_6)) return DEV_CHECK_LINE_6;
	else return DEV_CHECK_LINE_7;
}

/**
  * @brief Sblocca il processo sul semaforo del device che ha causato l'interrupt.
  *	   Se non c'è nessun processo da bloccare si aggiorna la matrice degli status word dei device,
  *	   altrimenti si inserisce nella readyQueue.
  * @param *semaddr : indirizzo del semaforo associato al device che ha causato l'interrupt
  * @param status : campo del device corrispondente al suo stato
  * @param line : linea di interrupt su cui è presente l'interrupt pendente da gestire
  * @param dev : device che ha causato l'interrupt pendente da gestire
  * @return Ritorna il pcb bloccato sul semaforo del device il cui indirizzo è passato per parametro, se presente
 */
HIDDEN pcb_t *verhogenInt(int *semaddr, int status, int line, int dev)
{
	pcb_t *p;
	
	(*semaddr)++;
	
	p=removeBlocked((S32 *)semaddr);
	/* Se non sono stati sbloccati dei processi, ritorna lo status Word del device */
	if(p == NULL)
		statusWordDev[line][dev] = status;
	/* Altrimenti ... */
	else {
		insertProcQ(&readyQueue, p);
		p->p_isOnDev = FALSE;
		softBlockCount--;
		p->p_state.reg_v0 = status;
	}
	
	return p;
}

/**
  * @brief Gestore degli Interrupts.
  * @return void.
 */
void intHandler()
{
	int cause_int;
	int *bitMapDevice;
	int devNumb;
	pcb_t *p;
	
	/* Se è presente un processo sulla CPU, carica la Interrupt Old Area su di esso */
	if(currentProcess != NULL)
		saveCurrentState(int_old_area, &(currentProcess->p_state));
	
	/* Recupera il contenuto del registro cause */
	cause_int = int_old_area->cause;
	
	/* Se la causa dell'interrupt è la linea 2 (la linea 0 e la 1 si ignorano poichè Kaya non genererà interrupt software) */
	if(CAUSE_IP_GET(cause_int, INT_TIMER))
	{
		/* Aggiorna il tempo trascorso */
		timerTick += (GET_TODLOW - startTimerTick);
		startTimerTick = GET_TODLOW;
		
		/* Se è arrivato l'interrupt dallo pseudo-clock */
		if(timerTick >= SCHED_PSEUDO_CLOCK)
		{
			/* Se sono state fatte più SYS7 precedentemente */
			if(pseudo_clock < 0)
			{
				/* Sblocca tutti i processi bloccati */
				while(pseudo_clock < 0)
				{
					p = removeBlocked(&pseudo_clock);
					/* Se sono stati sbloccati dei processi ... */
					if(!(p == NULL))
					{ 	/* Se pseudo_clock < 0 deve esserci almeno un processo bloccato */
						insertProcQ(&readyQueue, p);
						p->p_isOnDev = FALSE;
						softBlockCount--;
					}
					pseudo_clock++;
				}
			}
			else
			{
				p = removeBlocked(&pseudo_clock);
				/* Se non viene sbloccato nessun processo (pseudo-V), decrementa lo pseudo-clock */
				if(p == NULL) pseudo_clock--;
				/* Altrimenti esegue la V sullo pseudo-clock */
				else
				{
					insertProcQ(&readyQueue, p);
					p->p_isOnDev = FALSE;
					softBlockCount--;
					pseudo_clock++;
				}
			}
			
			/* Riavvia il tempo per il calcolo dello pseudo-clock tick */
			timerTick = 0;
			startTimerTick = GET_TODLOW;
		}
		/* Se è finito il timeslice del processo corrente */
		else if(currentProcess != NULL)
		{
			/* Reinserisce il processo nella Ready Queue */
			insertProcQ(&readyQueue, currentProcess);

			currentProcess = NULL;
			softBlockCount++;
			
			/* Aggiorna il tempo trascorso */
			timerTick += (GET_TODLOW - startTimerTick);
			startTimerTick = GET_TODLOW;
		}
		/* Altre cause */
		else
			SET_IT(SCHED_PSEUDO_CLOCK - timerTick);
	}
	/* Se la causa dell'interrupt è la linea 3 */
	else if(CAUSE_IP_GET(cause_int, INT_DISK))
	{	
		/* Dall'indirizzo iniziale delle bitmap degli interrupt pendenti si accede a quella corrispondente alla propria linea */
		bitMapDevice =(int *) (PENDING_BITMAP_START + (WORD_SIZE * (INT_DISK - DEV_DIFF)));
		
		/* Cerca all'interno della bitmap di questa specifica linea qual è il device con priorità più alta con un interrupt pendente */
		devNumb = recognizeDev(*bitMapDevice);
		
		/* Cerca l'indirizzo del device */
		devAddrBase = (memaddr) (DEV_REGS_START + ((INT_DISK - DEV_DIFF) * CHECK_EIGHTH_BIT) + (devNumb * CHECK_FIFTH_BIT));
		
		/* Recupera il campo status del device */
		status = (int*) (devAddrBase + DEV_REG_STATUS);
		/* Recupera il campo command del device */
		command = (int *) (devAddrBase + DEV_REG_COMMAND);
	
		/* Compie una V sul semaforo associato al device che ha causato l'interrupt */
		p = verhogenInt(&sem.disk[devNumb], (*status), INT_DISK - DEV_DIFF, devNumb);
		
		/* ACK per il riconoscimento dell'interrupt pendente */
		(*command) = DEV_C_ACK;
	}
	/* Se la causa dell'interrupt è la linea 4 */
	else if(CAUSE_IP_GET(cause_int, INT_TAPE))
	{
		/* Dall'indirizzo iniziale delle bitmap degli interrupt pendenti si accede a quella corrispondente alla propria linea */
		bitMapDevice =(int *) (PENDING_BITMAP_START + (WORD_SIZE * (INT_TAPE - DEV_DIFF)));
		
		/* Cerca all'interno della bitmap di questa specifica linea qual è il device con priorità più alta con un interrupt pendente */
		devNumb = recognizeDev(*bitMapDevice);
		
		/* Cerca l'indirizzo del device */
		devAddrBase = (memaddr) (DEV_REGS_START + ((INT_TAPE - DEV_DIFF) * CHECK_EIGHTH_BIT) + (devNumb * CHECK_FIFTH_BIT));
		
		/* Recupera il campo status del device */
		status = (int*) (devAddrBase + DEV_REG_STATUS);
		/* Recupera il campo command del device */
		command = (int *) (devAddrBase + DEV_REG_COMMAND);
		
		/* Compie una V sul semaforo associato al device che ha causato l'interrupt */
		p = verhogenInt(&sem.tape[devNumb], (*status), INT_TAPE - DEV_DIFF, devNumb);
		
		/* ACK per il riconoscimento dell'interrupt pendente */
		(*command) = DEV_C_ACK;
	}
	/* Se la causa dell'interrupt è la linea 5 */
	else if(CAUSE_IP_GET(cause_int, INT_UNUSED))
	{
		/* Dall'indirizzo iniziale delle bitmap degli interrupt pendenti si accede a quella corrispondente alla propria linea */
		bitMapDevice =(int *) (PENDING_BITMAP_START + (WORD_SIZE * (INT_UNUSED - DEV_DIFF)));
		
		/* Cerca all'interno della bitmap di questa specifica linea qual è il device con priorità più alta con un interrupt pendente */
		devNumb = recognizeDev(*bitMapDevice);
		
		/* Cerca l'indirizzo del device */
		devAddrBase = (memaddr) (DEV_REGS_START + ((INT_UNUSED - DEV_DIFF) * CHECK_EIGHTH_BIT) + (devNumb * CHECK_FIFTH_BIT));
		
		/* Recupera il campo status del device */
		status = (int*) (devAddrBase + DEV_REG_STATUS);
		/* Recupera il campo command del device */
		command = (int *) (devAddrBase + DEV_REG_COMMAND);
		
		/* Compie una V sul semaforo associato al device che ha causato l'interrupt */
		p = verhogenInt(&sem.network[devNumb], (*status), INT_UNUSED - DEV_DIFF, devNumb);
		
		/* ACK per il riconoscimento dell'interrupt pendente */
		(*command) = DEV_C_ACK;
	}
	/* Se la causa dell'interrupt è la linea 6 */
	else if(CAUSE_IP_GET(cause_int, INT_PRINTER))
	{
		/* Dall'indirizzo iniziale delle bitmap degli interrupt pendenti si accede a quella corrispondente alla propria linea */
		bitMapDevice =(int *) (PENDING_BITMAP_START + (WORD_SIZE * (INT_PRINTER - DEV_DIFF)));
		
		/* Cerca all'interno della bitmap di questa specifica linea qual è il device con priorità più alta con un interrupt pendente */
		devNumb = recognizeDev(*bitMapDevice);
		
		/* Cerca l'indirizzo del device */
		devAddrBase = (memaddr) (DEV_REGS_START + ((INT_PRINTER - DEV_DIFF) * CHECK_EIGHTH_BIT) + (devNumb * CHECK_FIFTH_BIT));
		
		/* Recupera il campo status del device */
		status = (int*) (devAddrBase + DEV_REG_STATUS);
		/* Recupera il campo command del device */
		command = (int *) (devAddrBase + DEV_REG_COMMAND);
		
		/* Compie una V sul semaforo associato al device che ha causato l'interrupt */
		p = verhogenInt(&sem.printer[devNumb], (*status), INT_PRINTER - DEV_DIFF, devNumb);
		
		/* ACK per il riconoscimento dell'interrupt pendente */
		(*command) = DEV_C_ACK;
	}
	/* Se la causa dell'interrupt è la linea 7 */
	else if(CAUSE_IP_GET(cause_int, INT_TERMINAL))
	{	
		/* Dall'indirizzo iniziale delle bitmap degli interrupt pendenti si accede a quella corrispondente alla propria linea */
		bitMapDevice =(int *) (PENDING_BITMAP_START + (WORD_SIZE * (INT_TERMINAL - DEV_DIFF)));
		
		/* Cerca all'interno della bitmap di questa specifica linea qual è il device con priorità più alta con un interrupt pendente */
		devNumb = recognizeDev(*bitMapDevice);
		
		/* Cerca l'indirizzo del device */
		devAddrBase = (memaddr) (DEV_REGS_START + ((INT_TERMINAL - DEV_DIFF) * CHECK_EIGHTH_BIT) + (devNumb * CHECK_FIFTH_BIT));
		
		/* Recupera il campo status del device (ricezione) */
		rStatus     = (int *) (devAddrBase + TERM_R_STATUS);
		/* Recupera il campo status del device (trasmissione) */
		tStatus   = (int *) (devAddrBase + TERM_T_STATUS);
		/* Recupera il campo command del device (ricezione) */
		rCommand    = (int *) (devAddrBase + TERM_R_COMMAND);
		/* Recupera il campo command del device (trasmissione) */
		tCommand  = (int *) (devAddrBase + TERM_T_COMMAND);
		
		/* Estrae il byte dello status per capire cosa è avvenuto */
		recvStatByte   = (*rStatus) & CHECK_STATUS_BIT;
		transmStatByte = (*tStatus) & CHECK_STATUS_BIT;

		/* Se è un carattere trasmesso */
		if(transmStatByte == DEV_TTRS_S_CHARTRSM)
		{
			/* Compie una V sul semaforo associato al device che ha causato l'interrupt */
			p = verhogenInt(&sem.terminalT[devNumb], (*tStatus), INT_TERMINAL - DEV_DIFF, devNumb);
			/* ACK per il riconoscimento dell'interrupt pendente */
			(*tCommand) = DEV_C_ACK;
		}
		
		/* Se è un carattere ricevuto */
		else if(recvStatByte == DEV_TRCV_S_CHARRECV)
		{
			/* Compie una V sul semaforo associato al device che ha causato l'interrupt */
			p = verhogenInt(&sem.terminalR[devNumb], (*rStatus), (INT_TERMINAL - DEV_DIFF) + 1, devNumb);
			/* ACK per il riconoscimento dell'interrupt pendente */
			(*rCommand) = DEV_C_ACK;
		}
	}
	
	scheduler();
}

#endif
