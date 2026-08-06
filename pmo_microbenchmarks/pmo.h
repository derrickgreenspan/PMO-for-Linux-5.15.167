
#ifndef __PMO_HEADER
#define __PMO_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
/* Copyright Derrick Greenspan and the University
 * of Central Florida -- all rights reserved */
static inline uint64_t djb2_hash(unsigned char *str)
{
        uint64_t hash = 5381;
	int c;
	while( c = *str++)
		hash = ((hash << 5) + hash) + c; /*  hash * 33 + c */   

	return hash;
}

static inline void * secure_attach(char *object, char command, char *key)
{
	uint64_t addr;
        syscall(548, object, command, key, 0, 0, &addr);
	return (void *) addr;
}

static inline void * dynamic_attach(char *object, char command, char *key)
{
	uint64_t addr;
        syscall(548, object, command, key, 0, 0, &addr);
	return (void *) addr;
}

static inline void * pmo_attach(const char *object, const char command,
		const char *key)
{
	uint64_t addr;
        syscall(548, object, command, key, 0, 0, &addr);
	return (void *) addr;
}
static inline void * attach(const char *object, const char command, const char *key)
{
	uint64_t addr;
        syscall(548, object, command, key, 0, 0, &addr);
	return (void *) addr;
}



static inline int __attribute__((always_inline)) secure_write_detach(void *pointer)
{
	syscall(548, pointer, 'd', 0, 0, NULL);
	return 0;
}
static inline int __attribute__((always_inline)) dynamic_detach(void *pointer, char deadbeef)
{
	syscall(548, pointer, 'd', 0, 0, NULL);
	return 0;
}

static inline int __attribute__((always_inline)) pmo_detach(void *pointer)
{
	syscall(548, pointer, 'd', 0, 0, NULL);
	return 0;
}
static inline int __attribute__((always_inline)) detach(void *pointer)
{
	syscall(548, pointer, 'd', 0, 0, NULL);
	return 0;
}

static inline int __attribute__((always_inline)) psync_nochecksum(void *pointer)
{
	return syscall(549, pointer, 1);
}

static inline int __attribute__((always_inline)) psync(void *pointer)
{
        return syscall(549, pointer, 0);
}

static inline char * __attribute__((always_inline))pmo_get_name(void *pointer)
{
	char *name = (char *)malloc(sizeof(char) *32);;
        syscall(548, pointer, 'n', 0, &name);
	return name;
}


/* Helper functions */
static inline const char *do_create(const char *object,
		uint64_t size, const char *key)
{
	syscall(548, object, 'c', key, size);
        return key;
}


static inline const char * pmo_create(const char *object, uint64_t size,
		const char *key)
{
        const char *key_to_return = do_create(object, size, key);
        if(!key_to_return)
        {
		printf("Key creation failed or" 
	        "PMO already exists\n");
	         return NULL;
	}
	return key_to_return;
}

static inline char __attribute__((always_inline)) pmo_exists(const char *object)
{
	return syscall(548, object, 'e', 0, 0);  
}

static inline size_t pmo_get_size(const char *object)
{
	return syscall(548, object, 's', 0, 0);
}

static inline size_t pmo_enable_debug(void)
{
	return syscall(548, NULL, 'u', 0, 0);
}	

static inline size_t pmo_disable_debug(void)
{
	return syscall(548, NULL, 'i', 0, 0);
}


#endif
