#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
@file scheduler.h
@author
@note This module implements Kaya’s process scheduler and deadlock detector.
*/

#include "../../phase1/h/pcb.h"
#include "exceptions.h"
#include "initial.h"
#include "interrupts.h"
#include "libumps.h"

/**
@brief Process scheduler.
@return Void.
*/
void scheduler(void);

#endif
