#ifndef NODE_H_INCLUDED
#	define NODE_H_INCLUDED

#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "mem.h"
#include "realtype.h"

#define CIRCUIT_EXPORT    static inline


enum {
	COMP_WIRE=0, // no real component, just abstract wire.
	COMP_VOLTAGE_SRC,
	COMP_DC_CURRENT_SRC,
	//COMP_AC_CURRENT_SRC,
	COMP_RESISTOR,
	COMP_CAPACITOR,
	COMP_INDUCTOR,
	//COMP_DIODE,
	MAX_COMP_TYPES,
};

enum {
	MAX_NODES = 20,
	GND_IDX   =  0,
};

CIRCUIT_EXPORT size_t idx1D(size_t const i, size_t const j, size_t const n) {
	return (i*n) + j;
}

/// _builtin_popcountg()
CIRCUIT_EXPORT NO_NULLS size_t setup_matrix_ids(size_t const nodes, uint8_t(*const restrict n_to_m)[MAX_NODES], uint8_t(*const restrict m_to_n)[MAX_NODES]) {
	size_t n = 0;
	for( size_t i=1; i < MAX_NODES; i++ ) { /// start at 1 because ignoring ground node.
		if( nodes & (1 << i) ) {
			(*n_to_m)[i] = n;
			(*m_to_n)[n] = i;
			n++;
		}
	}
	return n;
}

CIRCUIT_EXPORT NO_NULLS rat_t *alloc_vec(struct TIBiStack *s, size_t const n) {
	rat_t *v = bistack_alloc_front_vec(s, n, sizeof *v);
	for( size_t i=0; i < n; i++ ) {
		v[i] = rat_zero();
	}
	return v;
}

/// for solving the conductance matrix.
/// credit to Andrew via https://blamsoft.com/gaussian_rref-elimination-c-code/
CIRCUIT_EXPORT void gaussian_rref(size_t const n, rat_t A[const restrict], rat_t v[const restrict]) {
	for( size_t k = 0; k < n-1; k++ ) {
		size_t const kk = idx1D(k, k, n);
		/// Partial pivot
		rat_t cur_max = rat_abs(A[kk]);
		size_t m = k;
		for( size_t i = k+1; i < n; i++ ) {
			size_t const ik = idx1D(i, k, n);
			rat_t const potential_max = rat_abs(A[ik]);
			if( rat_lt(cur_max, potential_max) ) {
				cur_max = potential_max;
				m = i;
			}
		}
		if( rat_lt(cur_max, rat_epsilon()) ) {
			continue;
		}
		if( m != k ) {
			rat_t const b_temp = v[k];
			v[k] = v[m];
			v[m] = b_temp;
			for( size_t j = k; j < n; j++ ) {
				size_t const kj = idx1D(k, j, n);
				size_t const mj = idx1D(m, j, n);
				rat_t const A_temp = A[kj];
				A[kj] = A[mj];
				A[mj] = A_temp;
			}
		}
		/// Forward elimination
		for( size_t i = k+1; i < n; i++ ) {
			size_t const ik = idx1D(i, k, n);
			rat_t const factor = rat_div(A[ik], A[kk]);
			for( size_t j = k+1; j < n; j++ ) {
				size_t const ij = idx1D(i, j, n);
				size_t const kj = idx1D(k, j, n);
				A[ij] = rat_sub(A[ij], rat_mul(factor, A[kj]));
			}
			v[i] = rat_sub(v[i], rat_mul(factor, v[k]));
		}
	}
	/// Back substitution
	for( size_t i = n-1; i < n; i-- ) {
		size_t const ii = idx1D(i, i, n);
		for( size_t j = i+1; j < n; j++ ) {
			size_t const ij = idx1D(i, j, n);
			v[i] = rat_sub(v[i], rat_mul(A[ij], v[j]));
		}
		v[i] = rat_div(v[i], A[ii]);
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
	rat_t        val, current;
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
	rat_t            voltage[MAX_NODES];
	size_t           active_nodes; /// bitflagged.
};

CIRCUIT_EXPORT struct Circuit circuit_make(uint8_t *const memory, size_t const memory_size) {
	struct Circuit circ = {0};
	for( size_t i=0; i < MAX_NODES; i++ ) {
		circ.comps[i] = NULL;
	}
	circ.bistack = bistack_make(memory, memory_size);
	circ.voltage[GND_IDX] = rat_zero();
	return circ;
}

CIRCUIT_EXPORT NO_NULLS void circuit_reset_voltages(struct Circuit *const c) {
	for( size_t i=0; i < MAX_NODES; i++ ) {
		c->voltage[i] = rat_zero();
	}
}

CIRCUIT_EXPORT NO_NULLS void circuit_loop_components(
	struct Circuit *const restrict c,
	uint8_t         const node,
	void            action(struct Circuit *c, uint8_t node, struct Comp *comp, void *data),
	void            *const restrict data
) {
	for( struct Comp *comp = c->comps[node]; comp != NULL; comp = comp->next ) {
		action(c, node, comp, data);
	}
}

CIRCUIT_EXPORT NO_NULLS void circuit_connect_component(
	struct Circuit *const c,
	uint8_t         const n1,
	uint8_t         const n2,
	struct Comp    *const comp
) {
	comp->owner  = n1;
	comp->next   = c->comps[n1];
	c->comps[n1] = comp;
	
	struct Comp *comp_copy = component_new(&c->bistack, comp->val, comp->kind, n1);
	if( comp_copy==NULL ) {
		return;
	}
	comp_copy->owner = n2;
	comp_copy->next  = c->comps[n2];
	c->comps[n2]     = comp_copy;
	c->active_nodes |= ((1 << n1) | (1 << n2));
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
	circuit_connect_component(c, n1, n2, comp);
	return ERR_OK;
}

CIRCUIT_EXPORT NO_NULLS void circuit_calc_voltages(struct Circuit *const c) {
	circuit_reset_voltages(c);
	uint8_t node_to_matrix_idx[MAX_NODES] = {0};
	uint8_t matrix_idx_to_node[MAX_NODES] = {0};
	size_t const n = setup_matrix_ids(c->active_nodes, &node_to_matrix_idx, &matrix_idx_to_node);
	rat_t *G = alloc_vec(&c->bistack, n*n);
	rat_t *I = alloc_vec(&c->bistack, n);
	
	for( size_t idx_i=0; idx_i < n; idx_i++ ) {
		uint_fast8_t const node_i = matrix_idx_to_node[idx_i];
		for( struct Comp *comp = c->comps[node_i]; comp != NULL; comp = comp->next ) {
			uint_fast8_t const node_j = comp->node;
			int_fast8_t const idx_j = ( node_j != GND_IDX )? node_to_matrix_idx[node_j] : -1;
			switch( comp->kind ) {
				case COMP_RESISTOR: {
					rat_t const G_ij = rat_recip(comp->val); // Compute conductance G_ij = [1/R]
					// idx_i and idx_j correspond to node_i and node_j in the matrix
					if( node_i != GND_IDX ) {
						// Update diagonal element G[i][i]
						size_t const ii = idx1D(idx_i, idx_i, n);
						G[ii] = rat_add(G[ii], G_ij);
						if( idx_j != -1 ) {
							// Update off-diagonal elements
							size_t const ij = idx1D(idx_i, idx_j, n);
							size_t const ji = idx1D(idx_j, idx_i, n);
							size_t const jj = idx1D(idx_j, idx_j, n);
							G[ij] = rat_sub(G[ij], G_ij);
							G[ji] = rat_sub(G[ji], G_ij);
							G[jj] = rat_add(G[jj], G_ij); /// Update diagonal element G[j][j]
						} else {
							// node_j is ground
							// No need to update G[j][j] since ground node is not included in matrix
							// Adjust only G[i][i] and G[i][i] (already done)
						}
					} else if( node_j != GND_IDX ) {
						size_t const jj = idx1D(idx_j, idx_j, n);
						G[jj] = rat_add(G[jj], G_ij);
					}
					break;
				}
				case COMP_DC_CURRENT_SRC: {
					rat_t const I_s = comp->val;
					if( node_i != GND_IDX ) {
						size_t const idx_i = node_to_matrix_idx[node_i];
						I[idx_i] = rat_sub(I[idx_i], I_s);
					}
					if( node_j != GND_IDX ) {
						size_t const idx_j = node_to_matrix_idx[node_j];
						I[idx_j] = rat_add(I[idx_j], I_s);
					}
					break;
				}
				/// x = a ^ b ^ x; -> if( x==a ) x=b; else if( x==b ) x=a;
				/// if( node_i==GND_IDX ) node = node_j; else if( node_j==GND_IDX ) node = node_i;
				case COMP_VOLTAGE_SRC: {
					if( node_i==GND_IDX || node_j==GND_IDX ) {
						size_t const non_gnd_node = node_i ^ node_j ^ GND_IDX;
						size_t const non_gnd_id = node_to_matrix_idx[non_gnd_node];
						c->voltage[non_gnd_node] = comp->val;
						// Modify G and I to set V_node_j = comp->val
						for (size_t k = 0; k < n; k++) {
							size_t const non_gnd_k = idx1D(non_gnd_id, k, n);
							G[non_gnd_k] = rat_zero(); // Zero out the row
						}
						size_t const non_gnd_2 = idx1D(non_gnd_id, non_gnd_id, n);
						G[non_gnd_2] = rat_pos1(); // Set diagonal to 1
						I[non_gnd_id] = comp->val; // Set the known voltage
					} else {
						/// Voltage source between two non-ground nodes
					}
					break;
				}
			}
		}
	}
	
	gaussian_rref(n, G, I);
	for( size_t i=0; i < n; i++ ) {
		c->voltage[matrix_idx_to_node[i]] = I[i];
	}
	bistack_reset_front(&c->bistack);
}
#endif