#ifndef MEM_H_INCLUDED
#	define MEM_H_INCLUDED

#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define TI_MEM_EXPORT    static inline

#ifndef NO_NULLS
#	undef  NO_NULLS
#	define NO_NULLS         __attribute__((nonnull))
#endif

#ifndef EXTANT
#	undef  EXTANT
#	define EXTANT(...)      __attribute__( (nonnull(__VA_ARGS__)) )
#endif

#ifndef EXTANT_RET
#	undef  EXTANT_RET
#	define EXTANT_RET       __attribute__((returns_nonnull))
#endif


TI_MEM_EXPORT size_t _align_size(size_t const size, size_t const align) {
	return (size + (align - 1)) & ~(align - 1);
}


/** Region Allocator */
struct TIMemRegion {
	uint8_t *mem;
	size_t   offs, len;
};

TI_MEM_EXPORT NO_NULLS struct TIMemRegion region_make(uint8_t *const buf, size_t const len) {
	return (struct TIMemRegion){ .mem = buf, .len = len, .offs = 0 };
}

TI_MEM_EXPORT NO_NULLS void region_reset(struct TIMemRegion *const r) {
	r->offs = 0;
}

TI_MEM_EXPORT NO_NULLS void *region_alloc(struct TIMemRegion *const r, size_t bytes) {
	bytes = _align_size(bytes, sizeof bytes);
	if( r->offs + bytes >= r->len ) {
		return NULL;
	}
	size_t const alloc_offs = r->offs;
	r->offs += bytes;
	return memset(r->mem + alloc_offs, 0, bytes);
}

TI_MEM_EXPORT NO_NULLS void *region_alloc_reset_when_full(struct TIMemRegion *const r, size_t bytes, bool *const restrict was_reset) {
	bytes = _align_size(bytes, sizeof bytes);
	if( r->offs + bytes >= r->len ) {
		r->offs = 0;
		*was_reset = true;
	}
	size_t const alloc_offs = r->offs;
	r->offs += bytes;
	return memset(r->mem + alloc_offs, 0, bytes);
}

TI_MEM_EXPORT NO_NULLS ptrdiff_t region_size_remaining(struct TIMemRegion const *const r) {
	return ( ptrdiff_t )(r->len - r->offs);
}


/** Bifurcated/Double-Ended Stack */
struct TIBiStack {
	uint8_t *mem;
	size_t   front, back, len;
};


TI_MEM_EXPORT NO_NULLS struct TIBiStack bistack_make(uint8_t *const buf, size_t const len) {
	return (struct TIBiStack){ .mem = buf, .len = len, .front = 0, .back = 0 };
}

TI_MEM_EXPORT NO_NULLS void bistack_reset(struct TIBiStack *s) {
	s->front = 0;
	s->back = s->len;
}

TI_MEM_EXPORT NO_NULLS void bistack_reset_front(struct TIBiStack *const s) {
	s->front = 0;
}
TI_MEM_EXPORT NO_NULLS void bistack_reset_back(struct TIBiStack *const s) {
	s->back = s->len;
}

TI_MEM_EXPORT NO_NULLS void *bistack_alloc_front(struct TIBiStack *const s, size_t bytes) {
	bytes = _align_size(bytes, sizeof bytes);
	if( s->front + bytes >= s->back ) {
		return NULL;
	}
	size_t const alloc_offs = s->front;
	s->front += bytes;
	return memset(s->mem + alloc_offs, 0, bytes);
}

TI_MEM_EXPORT NO_NULLS void *bistack_alloc_back(struct TIBiStack *const s, size_t bytes) {
	bytes = _align_size(bytes, sizeof bytes);
	if( s->back - bytes <= s->front ) {
		return NULL;
	}
	s->back -= bytes;
	return memset(s->mem + s->back, 0, bytes);
}

TI_MEM_EXPORT NO_NULLS ptrdiff_t bistack_get_margins(struct TIBiStack const *s) {
	return s->back - s->front;
}


struct TIBuffer {
	uint8_t *data;
	size_t   cap, len;
};

TI_MEM_EXPORT NO_NULLS struct TIBuffer buffer_make(size_t const start_cap, size_t const elem_size, void *alloc_func(void *data, size_t bytes), void *allocator_data) {
	return (struct TIBuffer){
		.data = alloc_func(allocator_data, start_cap * elem_size),
		.cap = start_cap,
		.len = 0,
	};
}


#endif