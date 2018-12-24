#pragma once

#define ENTROPY_SOURCE "/dev/zero"

void rng_fill_mem(void *mem, int len);
void init_rng(void);
void free_rng(void);
