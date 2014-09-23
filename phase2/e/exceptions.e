/*
@file exceptions.e
@brief External definitions for exceptions.c
*/
 
#ifndef EXCEPTIONS_E
#define EXCEPTIONS_E

#include "../../include/const.h"
#include "../../include/types.h"
#include "initial.e"

EXTERN void saveCurrentState(state_t *currentState, state_t *newState);
EXTERN void sysBpHandler();
EXTERN int createProcess(state_t *statep);
EXTERN void terminateProcess();
EXTERN void verhogen(int *semaddr);
EXTERN void passeren(int *semaddr);
EXTERN void specTrapVec(int type, state_t *stateOld, state_t *stateNew);
EXTERN cpu_t getCPUTime();
EXTERN void waitClock();
EXTERN unsigned int waitIO(int interruptLine, int deviceNumber, int reading);
EXTERN void tlbHandler();
EXTERN void pgmTrapHandler();

#endif
