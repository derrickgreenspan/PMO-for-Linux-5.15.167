/* Copyright Derrick Greenspan and the University of Central Florida
 * All rights reserved (c) 2021 */

/* Note: djb2_hash was originally created by Dan Bernstein in the 1990s, 
 * but I do not know whether or not it is copyrighted; 
 * I suspect not since he has released it to the community,
 * to be extra safe, I can send him an email. */

#ifndef __PMO_STRUCTURES_LIB_H
#define __PMO_STRUCTURES_LIB_H
#include "pmo_range.h"

#define MAX_NODES 16777215
#define __u64 uint64_t
#define __u32 uint32_t

struct pmo_sha256 {
	char sum[32];
};

/*  http://www.cse.yorku.ca/~oz/hash.html */
__u64 djb2_hash(unsigned char *str)
{
	__u64 hash = 5381;
	int c;
	while( c = *str++)
		hash = ((hash << 5) + hash) + c; /*  hash * 33 + c */	
	return hash;
}

/* Macros */
#define IS_WRITE(x) test_and_set_bit(1, x)
#define IS_READ_OR_EXECUTE(x) !test_and_clear_bit(1, x)

#define IS_OCCUPIED(x) test_and_set_bit(0, x)
#define IS_UNOCCUPIED(x) test_and_clear_bit(0, x)

struct pmo_entry {

	/* Use atomic test_and_set_bit */
	/* 
	 * 1 0 - read/execute (treated the same) 
	 * 1 1 - write 
	 * 0 ** - unoccupied
	 *
	 * if LSB == 0 -- unoccupied
	 * if LSB == 1 -- occupied
	 *
	 * if 2nd bit = 1 -- write 
	 * if 2nd bit = 0 -- execute or read
	 *
	 * if 3rd bit = 1 -- in the process of memcpying 
	 * if 3rd bit = 0 -- data in primary is valid
       	*/


	/* Packing these things is annoying */
        char state; /* 1 byte */

        char name[27]; /* 56 bytes*/

        __u32 pm_size; /* 8 bytes */
        /* 64 bytes */

        /* Physical address of PMO from start of PMEM -- should be PFN */
        __u32 pm_primary; /* 8 bytes */
        __u32 pm_shadow; /* Location of the PMO shadow,
                                  Is 0 after detach*/

	__u32 pid; /* 4 bytes -- pid */
	__u32 boot_id; /* 4 bytes -- boot id */

        /*  16 bytes */


	char iv[16];
};

struct pmo_nodelist_s {
	__u64 allocated_nodes; /*  8 bytes */
	struct pmo_entry nodes[MAX_NODES]; 
	/* 16777215 * 128 = 2GiB - 128 */
};

typedef union {
	struct pmo_nodelist_s this;
	char padding[0x80000000]; /*  2GiB */
}pmo_nodelist;

/* TODO: We would prefer to manually calculate this (for larger or smaller PM
 * systems). But I'm a lazy man, and this has taken so much time despite the 
 * fact that it's not at all an interesting research problem, so I give up for
 * now. */
struct sha256_pages {
	struct pmo_sha256 page_hash[0x4000000]; /* 2GiB */
};

struct pmo_database_s {
	char header[4];
	char name[16];
	struct range pmo_range;

	__u64 nodelist_location; /* The location of the nodelist offset 
				    from the starting address */
	__u64 sha256_region_location; /* The location of the sha256 region offset
					 from the starting address */
	__u64 pmo_region_location;
	__u64 next_free_pmo; /* The next free PMO */

	char dram_offloading; /* Instead of a shadow PMO on NVMM, 
				 enables DRAM offloading -- the shadow PMO 
				instead lives in the DRAM and psync() memcpys
				the changes back to the NVMM */
	__u32 boot_id; /* Atomic */
};

typedef union {
	struct pmo_database_s this;
	char padding[0x1000];
}pmo_database;

#endif 
