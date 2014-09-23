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
EXTERN pcb_t *ReadyQueue;
EXTERN pcb_t *CurrentProcess;
EXTERN U32 ProcessCount;
EXTERN U32 PidCount;
EXTERN U32 SoftBlockCount;
EXTERN SemaphoreStruct Semaphore;
EXTERN int PseudoClock;
EXTERN cpu_t ProcessTOD;
EXTERN int StatusWordDev[STATUS_WORD_ROWS][STATUS_WORD_COLS];
EXTERN int TimerTick;
EXTERN cpu_t StartTimerTick;

#endif
