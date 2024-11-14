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
CIRCUIT_EXPORT void gaussian(size_t const n, rat_t **const restrict A, rat_t v[const restrict]) {
	for( size_t k = 0; k < n-1; k++ ) {
		/// Partial pivot
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
		/// Forward elimination
		for( size_t i = k+1; i < n; i++ ) {
			rat_t const factor = rat_div(A[i][k], A[k][k]);
			for( size_t j = k+1; j < n; j++ ) {
				A[i][j] = rat_sub(A[i][j], rat_mul(factor, A[k][j]));
			}
			v[i] = rat_sub(v[i], rat_mul(factor, v[k]));
		}
	}
	/// Back substitution
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
	
	uint8_t node_to_matrix_idx[MAX_NODES] = {0};
	uint8_t matrix_idx_to_node[MAX_NODES] = {0};
	size_t n = 0;
	for( uint8_t i=0; i < MAX_NODES; i++ ) {
		if( (c->active_nodes & (1 << i)) && i != GROUND_INDEX ) {
			node_to_matrix_idx[i] = n;
			matrix_idx_to_node[n] = i;
			n++;
		}
	}
	
	rat_t **G = bistack_alloc_front(&c->bistack, n * sizeof *G);
	for( size_t i=0; i < n; i++ ) {
		G[i] = bistack_alloc_front(&c->bistack, n * sizeof **G);
		for( size_t j=0; j < n; j++ ) {
			G[i][j] = rat_zero();
		}
	}
	
	rat_t *I = bistack_alloc_front(&c->bistack, n * sizeof *I);
	for( size_t i=0; i < n; i++ ) {
		I[i] = rat_zero();
	}
	
	for( size_t idx=0; idx < n; idx++ ) {
		uint8_t const node_i = matrix_idx_to_node[idx];
		for( struct Comp *comp = c->comps[node_i]; comp != NULL; comp = comp->next ) {
			uint8_t const node_j = comp->node;
			int const idx_j = ( node_j != GROUND_INDEX )? node_to_matrix_idx[node_j] : -1;
			switch( comp->kind ) {
				case DEVICE_RESISTOR: {
					// Conductance G = 1/R
					rat_t const G_ij = rat_recip(comp->val);
					// Diagonal element
					G[idx][idx] = rat_add(G[idx][idx], G_ij);
					// Off-diagonal elements
					if( idx_j != -1 ) {
						G[idx][idx_j] = rat_sub(G[idx][idx_j], G_ij);
						// Since G is symmetric
						G[idx_j][idx] = rat_sub(G[idx_j][idx], G_ij);
						G[idx_j][idx_j] = rat_add(G[idx_j][idx_j], G_ij);
					}
					break;
				}
				case DEVICE_DC_CURRENT_SRC: {
					// Current source from node_i to node_j
					rat_t I_s = comp->val;
					// Current leaving node_i
					I[idx] = rat_sub(I[idx], I_s);
					// Current entering node_j
					if( idx_j != -1 ) {
						I[idx_j] = rat_add(I[idx_j], I_s);
					}
					break;
				}
				case DEVICE_VOLTAGE_SRC: {
					// Voltage source handling (connected to ground)
					if( node_j==GROUND_INDEX ) {
						c->voltage[node_i] = comp->val;
						// Modify G and I to reflect known voltage
						for( size_t k=0; k < n; k++ ) {
							G[idx][k] = rat_zero();
							G[k][idx] = rat_zero();
						}
						G[idx][idx] = rat_pos1();
						I[idx] = comp->val;
					} else {
						// Voltage source between two non-ground nodes
						// Requires Modified Nodal Analysis
						// Not implemented here
					}
					break;
				}
			}
		}
	}
	
	rat_t *const v = I; // The RHS vector
	gaussian(n, G, v);
	for( size_t idx = 0; idx < n; idx++ ) {
		c->voltage[ matrix_idx_to_node[idx] ] = v[idx];
	}
	
	// Set ground node voltage
	c->voltage[GROUND_INDEX] = rat_zero();
	bistack_reset_front(&c->bistack);
}

#endif