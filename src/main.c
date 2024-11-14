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
	
	circuit_add_component(&circuit, 0, 1, DEVICE_VOLTAGE_SRC, rat_from_int(5)); // V1 between node 0 and 1
	circuit_add_component(&circuit, 1, 2, DEVICE_RESISTOR, rat_from_int(1000)); // R1 between node 1 and 2
	circuit_add_component(&circuit, 2, 0, DEVICE_RESISTOR, rat_from_int(1000)); // R2 between node 2 and 0
	
	circuit_calc_voltages(&circuit);
	for( size_t i=0; i < MAX_NODES; i++ ) {
		if( circuit.active_nodes & (1 << i) ) {
			char buffer[32] = {0};
			rat_to_str(circuit.voltage[i], sizeof buffer, buffer);
			printf("Voltage at node %zu: %s V\n", i, buffer);
		}
	}
	
#	ifdef __CE__
	os_ClrHome();                    /** Clear the homescreen */
	os_PutStrFull("Hello World");    /** Print a string  */
	for(;;) {
		if( os_GetCSC()==sk_Clear ) {
			break;
		}
	}
#	else
	printf("Hello World");
#	endif
}