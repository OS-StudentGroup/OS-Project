/*
@file interrupts.h
@note This module implements the device interrupt exception handler.
This module will process all the device interrupts, including Interval Timer interrupts,
converting device interrupts into V operations on the appropriate semaphores.
*/
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../e/scheduler.e"

/*
@brief Identify at which device a pending interrupt refers to.
@param bitmap Bitmap of the pending interrupt.
@return The device identifier.
*/
HIDDEN int getDevice(int bitmap);

/*
@brief Performs a V on the given device semaphore.
@param semaddr Address of the semaphore.
@param status Device status.
@return Void.
*/
HIDDEN void verhogenInt(int *semaddr, int status);

/*
@brief Acknowledge a pending interrupt on the Timer Click.
@return Void.
*/
HIDDEN void intTimer();

/**
@brief Acknowledge a pending interrupt through setting the command code in the device register.
@return Void.
*/
HIDDEN void devInterrupt(int cause);

/*
@brief Acknowledge a pending interrupt on the terminal, distinguishing between receiving and sending ones.
@return Void.
*/
HIDDEN void intTerminal();

/*
@brief The function identifies the pending interrupt and performs a V on the related semaphore.
@return Void.
*/
EXTERN void intHandler();



#endif
