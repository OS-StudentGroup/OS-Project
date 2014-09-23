/*
@file exceptions.h
@note This module implements the TLB PgmTrap and SYS/Bp exception handlers.
*/
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "../e/scheduler.e"

/**
@brief Save current state in a new state.
@param currentState Current state.
@param newState New state.
@return Void.
*/
EXTERN void saveCurrentState(state_t *currentState, state_t *newState);

/**
@brief Performs a P on a device semaphore and increments the Soft-Block count, i.e. increase
by one the number of processes in the system currently blocked and waiting for an interrupt.
@param semaddr Semaphore's address.
@return Void.
*/
HIDDEN void passerenIO(int *semaddr);

/**
@brief This function handles SYSCALL, distinguishing between User Mode and Kernel Mode.
@return Void.
*/
HIDDEN void systemCallHandler(int exceptionType, int statusMode);

/**
@brief This function handles Breakpoint exceptions, distinguishing the case in which the
SYS5 has not been yet executed, and the case in which it has.
@return Void.
*/
HIDDEN void breakPointHandler(int exceptionType, int statusMode)

/**
@brief This function handles SYSCALL or Breakpoint exceptions, which occurs when a SYSCALL or BREAK assembler
instruction is executed.
@return Void.
*/
EXTERN void sysBpHandler()

/**
@brief (SYS1) Create a new process.
@param statep Initial state for the newly created process.
@return -1 in case of failure; PID in case of success.
*/
EXTERN int createProcess(state_t *statep)

/**
@brief (SYS2) Terminate a process and all its progeny. In case a process in blocked on a semaphore, performs a V on it.
@return 0 in case of success; -1 in case of error.
*/
EXTERN void terminateProcess();

/*
@brief Recursively terminate a process and its progeny.
@param process Pointer to the process control block.
@return Void.
*/
HIDDEN void _terminateProcess(pcb_t *process);

/*
@brief (SYS3) Perform a V operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
EXTERN void verhogen(int *semaddr);

/*
@brief (SYS4) Perform a P operation on a semaphore.
@param semaddr Semaphore address.
@return Void.
*/
EXTERN void passeren(int *semaddr);

/*
@brief Specify Exception State Vector.
@param type Type of exception.
@param stateOld The address into which the old processor state is to be stored when an exception
occurs while running this process.
@param stateNew The processor state area that is to be taken as the new processor state if an
exception occurs while running this process.
@return Void.
*/
EXTERN void specTrapVec(int type, state_t *stateOld, state_t *stateNew);

/*
@brief (SYS6) Retrieve the CPU time of the current process.
@return CPU time of the current process.
*/
EXTERN cpu_t getCPUTime()

/*
@brief (SYS7) Performs a P on the pseudo-clock semaphore.
@return Void.
*/
EXTERN void waitClock();

/**
@brief (SYS8) Perform a P operation on a device semaphore.
@param interruptLine Interrupt line.
@param deviceNumber Device number.
@param reading : TRUE in case of reading; FALSE else.
@return Return the device's Status Word.
*/
EXTERN unsigned int waitIO(int interruptLine, int deviceNumber, int reading);

/*
@brief TLB (Translation Look Aside Buffer) Exception Handling.
@return void.
*/
EXTERN void tlbHandler();

/*
@brief Program Trap handler.
@return Void.
*/
EXTERN void pgmTrapHandler();

#endif
