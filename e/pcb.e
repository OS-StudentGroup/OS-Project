/* Externals declaration file for the PCB
 * 
 * This will insure that the ASL can only use the externally 
 * visible functions from PCB for maintaining its process queues.
*/
#ifndef PCB_E
#define PCB_E

// Dependencies
#include "../h/pcb.h"

// External function declarations
EXTERN pcb_t *mkEmptyProcQ(void);
EXTERN int emptyProcQ(pcb_t *tp);
EXTERN void insertProcQ(pcb_t **tp, pcb_t *p);
EXTERN pcb_t *removeProcQ(pcb_t **tp);
EXTERN pcb_t *outProcQ(pcb_t **tp, pcb_t *p);
EXTERN pcb_t *headProcQ(pcb_t *tp);

#endif