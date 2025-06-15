#pragma once

// If dst is equal to cmp, set dst to val, otherwise do nothing, return dst before op.
void *atomic_cmp_swap(void **dst, void *val, void *cmp);
void *atomic_swap(void **dst, void *val);
s32	atomic_cmp_swap(s32 *dst, s32 val, s32 cmp);
s32	atomic_swap(s32 *dst, s32 val); // swap dst and val and return dst before op
s32	atomic_add(s32 *dst, s32 val); // add val to dst and return dst before op
s32	atomic_increment(s32 *dst);
s32	atomic_decrement(s32 *dst);
