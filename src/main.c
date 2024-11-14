#include <tice.h>
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


enum{ MEM_SIZE = 1 << 12 };
uint8_t backing_mem[MEM_SIZE];

int main(void) {
#	ifdef TICE_H
#	warning "compiling for TI Calculator"
	os_ClrHome();
	puts("Welcome to LiteSpiCE");
	struct Circuit circuit = circuit_make(backing_mem, sizeof backing_mem);
	if( circuit_add_component(&circuit, 0, 1, DEVICE_VOLTAGE_SRC, rat_from_int(5))==ERR_OK ) {  // V1 between node 0 and 1
		puts("Voltage Src (5 volts) | ground -> node 1.");
	}
	if( circuit_add_component(&circuit, 1, 2, DEVICE_RESISTOR, rat_from_int(1000)) ) { // R1 between node 1 and 2
		puts("Resistor (1K ohm) | node 1 -> node 2.");
	}
	if( circuit_add_component(&circuit, 2, 0, DEVICE_RESISTOR, rat_from_int(2000)) ) { // R2 between node 2 and 0
		puts("Resistor (2K ohm) | node 2 -> ground.");
	}
	puts("Calcing voltages...");
	circuit_calc_voltages(&circuit);
	while( os_GetCSC() != sk_Enter );
	os_ClrHome();
	puts("Done calculating voltages...\n\nPrinting Voltages::");
	for( size_t i=0; i < MAX_NODES; i++ ) {
		if( circuit.active_nodes & (1 << i) ) {
			char voltage_str[32] = {0}; rat_to_str(circuit.voltage[i], sizeof voltage_str, voltage_str);
			printf("Node_%zu: %s volts\n", i, voltage_str);
		}
	}
	while( os_GetCSC() != sk_Clear );
#	else
#	warning "compiling for PC/Other"
	puts("Welcome to LiteSpiCE");
	struct Circuit circuit = circuit_make(backing_mem, sizeof backing_mem);
	if( circuit_add_component(&circuit, 0, 1, DEVICE_VOLTAGE_SRC, rat_from_int(5))==ERR_OK ) {  // V1 between node 0 and 1
		puts("Voltage Src (5 volts) | ground -> node 1.");
	}
	if( circuit_add_component(&circuit, 1, 2, DEVICE_RESISTOR, rat_from_int(1000)) ) { // R1 between node 1 and 2
		puts("Resistor (1K ohm) | node 1 -> node 2.");
	}
	if( circuit_add_component(&circuit, 2, 0, DEVICE_RESISTOR, rat_from_int(2000)) ) { // R2 between node 2 and 0
		puts("Resistor (2K ohm) | node 2 -> ground.");
	}
	puts("Calcing voltages...");
	circuit_calc_voltages(&circuit);
	puts("Done calculating voltages...\n\nPrinting Voltages::");
	for( size_t i=0; i < MAX_NODES; i++ ) {
		if( circuit.active_nodes & (1 << i) ) {
			char voltage_str[32] = {0}; rat_to_str(circuit.voltage[i], sizeof voltage_str, voltage_str);
			printf("Node_%zu: %s volts\n", i, voltage_str);
		}
	}
#	endif
}