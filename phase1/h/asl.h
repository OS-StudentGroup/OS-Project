/* Semaphore list handling functions */
#ifndef ASL_H
#define ASL_H

/* Dependencies */
#include "../e/pcb.e"
#include "../../include/const.h"
#include "../../include/types.h"

/* External function declarations */
EXTERN void initASL(void);
EXTERN int insertBlocked(int *semAdd, pcb_t *p);
EXTERN pcb_t *removeBlocked(int *semAdd);
EXTERN pcb_t *outBlocked(pcb_t *p);
EXTERN pcb_t *headBlocked(int *semAdd);

#endif
