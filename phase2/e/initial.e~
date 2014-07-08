/**
 *  @file initial.e
 *  @author Vincenzo Ferrari - Barbara Iadarola
 *  @brief File di definizione del modulo initial.e
 *  @note Contiene tutte le definizioni delle funzioni implementate nel modulo initial.e
 */
 
#ifndef INITIAL_E
#define INITIAL_E

#include <types10.h>
#include <listx.h>
#include <const.h>

extern void test(void);

extern struct list_head readyQueue;

extern pcb_t *currentProcess;

extern U32 processCount;

extern U32 pidCount;

extern U32 softBlockCount;

extern struct {
	int disk[8];
	int tape[8];
	int network[8];
	int printer[8];
	int terminalR[8];
	int terminalT[8];
} sem;

extern int pseudo_clock;

extern cpu_t processTOD;

extern int statusWordDev[6][8];

extern int timerTick;

extern cpu_t startTimerTick;

extern pcb_pid_t pcbused_table[MAXPROC];

#endif
