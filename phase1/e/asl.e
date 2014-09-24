/* Externals declaration file for the Semaphore list
 * 
 * This will insure that the ASL can only use the externally 
 * visible functions from PCB for maintaining its process queues.
*/
#ifndef ASL_E
#define ASL_E

#include "../e/pcb.e"
#include "../../include/const.h"
#include "../../include/types.h"

EXTERN void initASL(void);
EXTERN int insertBlocked(int *semAdd, pcb_t *p);
EXTERN pcb_t *removeBlocked(int *semAdd);
EXTERN pcb_t *outBlocked(pcb_t *p);
EXTERN pcb_t *headBlocked(int *semAdd);

#endif
