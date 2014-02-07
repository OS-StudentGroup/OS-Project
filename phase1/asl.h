#ifndef ASL_H
#define ASL_H
/* Semaphore list handling functions */

#include "../phase1/pcb.h"
#include "../h/const.h"
#include "../h/types.h"

EXTERN void initSemd(void);
EXTERN int insertBlocked(int *semAdd, pcb_t *p);
EXTERN pcb_t *removeBlocked(int *semAdd);
EXTERN pcb_t *outBlocked(pcb_t *p);
EXTERN pcb_t *headBlocked(int *semAdd);

#endif

