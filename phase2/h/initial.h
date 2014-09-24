/**
@file initial.h
@note Entry point.
*/
#ifndef INITIAL_H
#define INITIAL_H

#include "../e/inclusions.e"

/* Global variables declarations */
EXTERN pcb_t *ReadyQueue;										/**< Process ready queue */
EXTERN pcb_t *CurrentProcess;									/**< Pointer to the executing PCB */
EXTERN U32 ProcessCount;										/**< Process counter */
EXTERN U32 PidCount;											/**< PID counter */
EXTERN U32 SoftBlockCount;										/**< Blocked process counter */
EXTERN SemaphoreStruct Semaphore;								/**< Device semaphores per line */
EXTERN int PseudoClock;											/**< Pseudo-clock semaphore */
EXTERN U32 ProcessTOD;										/**< Process start time */
EXTERN int TimerTick;											/**< Tick timer */
EXTERN U32 StartTimerTick;									/**< Pseudo-clock tick start time */

EXTERN void test(void);

/*
@brief Populate a new processor state area.
@param area Physical address of the area.
@param handler Physical address of the handler.
@return Void.
*/
HIDDEN void populateArea(memaddr area, memaddr handler);

/*
@brief The entry point for Kaya which performs the nucleus initialization.
@return Void.
*/
void main(void);

#endif
