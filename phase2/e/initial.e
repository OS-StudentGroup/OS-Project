/*
@file initial.e
@brief Esternal definitions for initial.c
*/
 
#ifndef INITIAL_E
#define INITIAL_E

#include "../../include/const.h"
#include "../../include/types.h"

EXTERN void test(void);

/* External global variables */
pcb_t *ReadyQueue;
pcb_t *CurrentProcess;
U32 ProcessCount;
U32 PidCount;
U32 SoftBlockCount;
SemaphoreStruct Semaphore;
int PseudoClock;
cpu_t ProcessTOD;
int StatusWordDev[STATUS_WORD_ROWS][STATUS_WORD_COLS];
int TimerTick;
cpu_t StartTimerTick;

#endif
