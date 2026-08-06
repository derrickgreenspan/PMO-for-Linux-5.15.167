#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
/* Copyright Derrick Greenspan and the University
 * of Central Florida -- all rights reserved */
unsigned long int get_size(char *object, char *key);

static inline uint64_t __inline __attribute__((always_inline)) djb2_hash(unsigned char *str)
{
        uint64_t hash = 5381;
	int c;
	while( c = *str++)
		hash = ((hash << 5) + hash) + c; /*  hash * 33 + c */   

	return hash;
}


static inline void * __attribute__((always_inline)) pmo_attach(char *object, char command, char *key)
{
	uint64_t addr;
        syscall(548, object, command, key, 0, 0, &addr);
	return (void *) addr;
}

/* Usage of this function is highly discouraged */
[[deprecated("For superior performance, use small PMOs instead of attaching subsets")]]
static inline void * __attribute__((always_inline)) subset_attach(char *object, char command, char *key,
	       	size_t size, size_t offset)
{
	uint64_t addr;
	syscall(548, object, command, key, size, offset, &addr);
	return (void *) addr;
}

static inline int __attribute__((always_inline)) pmo_detach(void *pointer)
{
	syscall(548, pointer, 'd', 0, 0, NULL);
	return 0;
}

static inline int __attribute__((always_inline)) psync(void *pointer)
{
        return syscall(549, pointer, 0);
}

/* Helper functions */
static inline __attribute__((always_inline)) char *do_create(char *object, uint64_t size, char *key)
{
	syscall(548, object, 'c', key, size);
        return key;
}


static inline char * __attribute__((always_inline)) pmo_create(char *object, uint64_t size, char *key)
{
        char *key_to_return = do_create(object, size, key);
        if(!key_to_return)
        {
		printf("Key creation failed or" 
	        "PMO already exists\n");
	         return NULL;
	}
	return key_to_return;
}

static inline char __attribute__((always_inline)) pmo_exists(char *object)
{
	return syscall(548, object, 'e', 0, 0);  
}
