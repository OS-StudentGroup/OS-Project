#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/**
@file interrupts.h
@author
@note This module implements the device interrupt exception handler. This module will process all the device interrupts, including Interval Timer interrupts, converting device interrupts into V operations on the appropriate semaphores.
*/

#include "../e/scheduler.e"

/* Variabili per accedere ai campi status e command di device o terminali */
HIDDEN int *rStatus, *tStatus;
HIDDEN int *rCommand, *tCommand;
HIDDEN int recvStatByte, transmStatByte;
HIDDEN int *status, *command;

/**
@brief Riconosce il device che ha un interrupt pendente sulla bitmap passata per parametro.
@param bitMapDevice : bitmap degli interrupt pendenti per linea di interrupt.
@return Ritorna l'indice del device che ha un interrupt pendente su quella bitmap.
*/
HIDDEN int recognizeDev(int bitMapDevice);

/**
@brief Sblocca il processo sul semaforo del device che ha causato l'interrupt.
Se non c'è nessun processo da bloccare si aggiorna la matrice degli status word dei device,
altrimenti si inserisce nella readyQueue.
@param *semaddr : indirizzo del semaforo associato al device che ha causato l'interrupt
@param status : campo del device corrispondente al suo stato
@param line : linea di interrupt su cui è presente l'interrupt pendente da gestire
@param dev : device che ha causato l'interrupt pendente da gestire
@return Ritorna il pcb bloccato sul semaforo del device il cui indirizzo è passato per parametro, se presente
*/
HIDDEN pcb_t *verhogenInt(int *semaddr, int status, int line, int dev);

/**
@brief Interrupts manager.
@return Void.
*/
void intHandler();

#endif
