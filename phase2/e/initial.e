/*
@file initial.e
@brief External definitions for initial.c
*/

#include "../../include/uARMconst.h"
#include "../../include/types.h"

EXTERN U32 ProcessCount;
EXTERN U32 SoftBlockCount;
EXTERN U32 ProcessTOD;
EXTERN U32 TimerTick;
EXTERN U32 StartTimerTick;
EXTERN pcb_t *ReadyQueue;
EXTERN pcb_t *CurrentProcess;
EXTERN DeviceSemaphores Semaphores;
EXTERN int PseudoClock;
