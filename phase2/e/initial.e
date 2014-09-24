/*
@file initial.e
@brief Esternal definitions for initial.c
*/

#include "../../include/types.h"
#include "../../include/uARMconst.h"

EXTERN pcb_t *ReadyQueue;
EXTERN pcb_t *CurrentProcess;
EXTERN U32 ProcessCount;
EXTERN U32 SoftBlockCount;
EXTERN SemaphoreStruct Semaphore;
EXTERN int PseudoClock;
EXTERN U32 ProcessTOD;
EXTERN int TimerTick;
EXTERN U32 StartTimerTick;