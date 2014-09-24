/* PCB handling functions */
#ifndef PCB_H
#define PCB_H

/* Dependencies */
#include "../../include/const.h"
#include "../../include/types.h"

/* External function declarations */
/* [1] List view functions */
EXTERN void freePcb(pcb_t *p);
EXTERN pcb_t *allocPcb(void);
EXTERN void initPcbs(void);
EXTERN pcb_t *mkEmptyProcQ(void);
EXTERN int emptyProcQ(pcb_t *tp);
EXTERN void insertProcQ(pcb_t **tp, pcb_t *p);
EXTERN pcb_t *removeProcQ(pcb_t **tp);
EXTERN pcb_t *outProcQ(pcb_t **tp, pcb_t *p);
EXTERN pcb_t *headProcQ(pcb_t *tp);

/* [2] Tree view functions */
EXTERN int emptyChild(pcb_t *p);
EXTERN void insertChild(pcb_t *prnt, pcb_t *p);
EXTERN pcb_t *removeChild(pcb_t *p);
EXTERN pcb_t *outChild(pcb_t *p);
#endif
