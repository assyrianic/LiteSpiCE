#include <ti/screen.h>
#include <ti/getcsc.h>
#include <stdlib.h>
#include "node.h"


/**
0 -> Ground/Reference node.
  1           2           3           4
  *---\/\/\---*---\/\/\---*---\/\/\---*
  |    R1     |    R2     |    R3     |
  |           |           |           |
__+__         >           >         __+__
 ___ V1       < R4        < R5       ___ V2
  |           >           >           |
  |           |           |           |
  |           |    R6     |    R7     |
  ------------*---\/\/\---*---\/\/\---*
              0           5           6
V1 -> [0][1]
R1 -> [1][2]
R2 -> [2][3]
R3 -> [3][4]
R4 -> [0][2]
R5 -> [3][5]
R6 -> [0][5]
R7 -> [5][6]
V2 -> [6][4]
 */



enum{ MEM_SIZE = 1 << 15 };
uint8_t backing_mem[MEM_SIZE];

int main(void) {
	struct Circuit circuit = circuit_make(backing_mem, sizeof backing_mem);
	
	os_ClrHome();                    /** Clear the homescreen */
	os_PutStrFull("Hello World");    /** Print a string */
	while( !os_GetCSC() );           /** Waits for a key */
}