----------------------------------------
List of the Team Members
----------------------------------------

[*] Tommaso Ognibene 	(tommaso.ognibene@studio.unibo.it)
[*] Neta Kedem			()
[*] Alessio				()
[*] Alessio Panipucci	()

----------------------------------------
UARM Core File Generation & Testing
----------------------------------------

[1] Open the terminal and reach the directory "p1".
[2] Run "make" in order to generate the UARM core file.
[3] Open UARM and add the obtained UARM core file.
[4] Execute the test.

----------------------------------------
Design Decisions & Computational Remarks
----------------------------------------

The testing and analysis shall verify that the design specifications (http://www.cs.unibo.it/~renzo/so/Kaya.pdf) have been met.

The following computational remarks are worth to mention:

[1] Concerning the PCB process queue:
The specifications propose a single circularly linked list, but also accept a double circularly linked list.
We tried both ways. Nonetheless, we reached the conclusion that adding a second pointer (called p_prev) wouldn't improve the overall computational complexity. Therefore, there is no point in wasting memory with a second pointer. 
			
[2] Concerning the PCB process tree:
The specifications propose a NULL-terminated single linearly linked list, but also accept a NULL-terminated double linearly linked list.
We tried both ways. Nonetheless, we would like to note that neither of them is the best solution.
Since the specifications state that the pointer p_child should point to the first child of the process, then the best solution would rather be a double circularly linked list.
In particular, the function EXTERN void insertChild(pcb_t *prnt, pcb_t *p) would cost:
	[*] O(1), with a double circularly linked list
	[*] O(n), with the data structures given by the specifications.
With n representing the number of children.
Reason: in the first case there is no need for any cycle, in the second case there is.
			
[2] Concerning the ASL:
The specifications require a sorted NULL-terminated single linearly linked list, suggesting the use of a dummy header. We followed this advice for both the ASL and the semdFree list. This way we made the code shorter and more readable. 
