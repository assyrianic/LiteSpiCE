#ifndef NODE_H_INCLUDED
#	define NODE_H_INCLUDED

#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <ti/real.h>
#include "mem.h"
#include "realtype.h"

#define CIRCUIT_EXPORT    static inline


enum {
	DEVICE_WIRE=0, // no real component, just abstract wire.
	DEVICE_VOLTAGE_SRC,
	DEVICE_DC_CURRENT_SRC,
	//DEVICE_AC_CURRENT_SRC,
	DEVICE_RESISTOR,
	DEVICE_CAPACITOR,
	DEVICE_INDUCTOR,
	//DEVICE_DIODE,
	MAX_DEVICE_TYPES,
};

enum {
	MAX_NODES    = 20,
	MATRIX_SIZE  = MAX_NODES * (MAX_NODES + 1),
	GROUND_INDEX =  0,
};

CIRCUIT_EXPORT size_t pop_count(size_t const x) {
	size_t count = 0;
	for( size_t i = x; i > 0; i >>= 1 ) {
		count += (i & 1);
	}
	return count;
}

/// for solving the conductance matrix.
/// credit to Andrew via https://blamsoft.com/gaussian-elimination-c-code/
CIRCUIT_EXPORT void gaussian(size_t const n, rat_t *restrict *restrict A, rat_t v[const restrict]) {
	for( size_t k = 0; k < n-1; k++ ) {
		// Partial pivot
		rat_t cur_max = rat_abs(A[k][k]);
		size_t m = k;
		for( size_t i = k+1; i < n; i++ ) {
			rat_t const potential_max = rat_abs(A[i][k]);
			if( rat_lt(cur_max, potential_max) ) {
				cur_max = potential_max;
				m = i;
			}
		}
		if( m != k ) {
			rat_t const b_temp = v[k];
			v[k] = v[m];
			v[m] = b_temp;
			for( size_t j = k; j < n; j++ ) {
				rat_t const A_temp = A[k][j];
				A[k][j] = A[m][j];
				A[m][j] = A_temp;
			}
		}
		// Forward elimination
		for( size_t i = k+1; i < n; i++ ) {
			rat_t const factor = rat_div(A[i][k], A[k][k]);
			for( size_t j = k+1; j < n; j++ ) {
				A[i][j] = rat_sub(A[i][j], rat_mul(factor, A[k][j]));
			}
			v[i] = rat_sub(v[i], rat_mul(factor, v[k]));
		}
	}
	// Back substitution
	for( size_t i = n-1; i < n; i-- ) {
		v[i] = v[i];
		for( size_t j = i+1; j < n; j++ ) {
			v[i] = rat_sub(v[i], rat_mul(A[i][j], v[j]));
		}
		v[i] = rat_div(v[i], A[i][i]);
	}
}



enum {
	ERR_NODE_OOB  = -2,
	ERR_OOM       = -1,
	ERR_SELF_LOOP =  0,
	ERR_OK        =  1,
};

/// represents a component that's connected to between two nodes.
struct Comp {
	struct Comp *next;
	rat_t        val;
	uint8_t      kind, node, owner;
};

CIRCUIT_EXPORT NO_NULLS struct Comp *component_new(struct TIBiStack *const s, rat_t const value, uint8_t const kind, uint8_t const node) {
	struct Comp *comp = bistack_alloc_back(s, sizeof *comp);
	if( comp==NULL ) {
		return NULL;
	}
	comp->val  = value;
	comp->node = node;
	comp->kind = kind;
	return comp;
}


struct Circuit {
	struct TIBiStack bistack;
	
	/// Electrical circuits are basically a network of comps with components in between.
	/// The best way to represent an electrical circuit is a graph.
	/// Specifically, an adjacency list type graph where vertices/comps contain an
	/// edge/weight that represents an electrical component.
	struct Comp     *comps[MAX_NODES];
	rat_t            voltage[MAX_NODES], current[MAX_NODES];
	size_t           active_nodes; /// bitflagged.
};

CIRCUIT_EXPORT struct Circuit circuit_make(uint8_t *const memory, size_t const memory_size) {
	struct Circuit circ = {0};
	for( size_t i=0; i < MAX_NODES; i++ ) {
		circ.comps[i] = NULL;
	}
	circ.bistack = bistack_make(memory, memory_size);
	circ.voltage[GROUND_INDEX] = rat_zero();
	return circ;
}

CIRCUIT_EXPORT NO_NULLS void circuit_connect_component(struct Circuit *const c, uint8_t const node, struct Comp *const comp) {
	comp->owner    = node;
	comp->next     = c->comps[node];
	c->comps[node] = comp;
	c->active_nodes |= (1 << node);
	c->active_nodes |= (1 << comp->node);
}

CIRCUIT_EXPORT NO_NULLS struct Comp *circuit_get_component(
	struct Circuit const *const c,
	uint8_t               const n1,
	uint8_t               const n2
) {
	for( struct Comp *comp = c->comps[n1]; comp != NULL; comp = comp->next ) {
		if( comp->node==n2 ) {
			return comp;
		}
	}
	return NULL;
}

CIRCUIT_EXPORT NO_NULLS int circuit_add_component(
	struct Circuit *const c,
	uint8_t         const n1,
	uint8_t         const n2,
	uint8_t         const comp_type,
	rat_t           const value
) {
	if( n1 >= MAX_NODES || n2 >= MAX_NODES ) {
		return ERR_NODE_OOB;
	} else if( n1==n2 ) {
		return ERR_SELF_LOOP;
	}
	
	struct Comp *comp = component_new(&c->bistack, value, comp_type, n2);
	if( comp==NULL ) {
		return ERR_OOM;
	}
	circuit_connect_component(c, n1, comp);
	return ERR_OK;
}

CIRCUIT_EXPORT NO_NULLS void circuit_calc_voltages(struct Circuit *const c) {
	for( size_t i=0; i < MAX_NODES; i++ ) {
		c->voltage[i] = rat_zero();
		c->current[i] = rat_zero();
	}
	rat_t const eps = rat_epsilon();
	for( size_t i=0; i < MAX_NODES; i++ ) {
		for( struct Comp *comp = c->comps[i]; comp != NULL; comp = comp->next ) {
			size_t const j = comp->node;
			switch( comp->kind ) {
				case DEVICE_VOLTAGE_SRC: {
					c->voltage[j] = comp->val;
					break;
				}
				case DEVICE_CAPACITOR: {
					/// 
					break;
				}
				case DEVICE_DC_CURRENT_SRC: {
					/// 
					break;
				}
				/*
				case DEVICE_AC_CURRENT_SRC: {
					/// 
					break;
				}
				*/
				/*
				case DEVICE_DIODE: {
					/// 
					break;
				}
				*/
				case DEVICE_INDUCTOR: {
					/// V = L[dI/dt]
					/// V = L[(I_f - I_i) / (t_f - t_i)]
					break;
				}
				case DEVICE_RESISTOR: {
					/**
					 * V = IR
					 * (V_n1 - V_n2) / R = I
					 * V_n2 = V_n1 - IR
					 */
					// If the voltage at one node is known, calculate the current
					if( !rat_eq(c->voltage[i], rat_zero(), eps) && !rat_eq(c->voltage[j], rat_zero(), eps) ) {
						// If both node voltages are known, calculate the current
						rat_t voltage_diff = rat_sub(c->voltage[i], c->voltage[j]);
						rat_t current = rat_div(voltage_diff, comp->val);
						c->current[i] = rat_add(c->current[i], current);
						c->current[j] = rat_sub(c->current[j], current);
					} else if (!rat_eq(c->voltage[i], rat_zero(), eps)) {
						// If voltage at node i is known, calculate voltage at node j
						c->voltage[j] = rat_sub(c->voltage[i], rat_mul(c->current[i], comp->val));
					} else if (!rat_eq(c->voltage[j], rat_zero(), eps)) {
						// If voltage at node j is known, calculate voltage at node i
						c->voltage[i] = rat_add(c->voltage[j], rat_mul(c->current[j], comp->val));
					}
					break;
				}
			}
		}
	}
}

#endif