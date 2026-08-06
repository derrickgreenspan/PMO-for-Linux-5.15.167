#include <fcntl.h>
#include <libpmem.h>
#include <pmo.h>

#ifdef NOVA 
	int create_snapshot = -1;
#endif

void *alloc_memory(size_t size, char * name)
{
        void *mem;

        #if defined(NOVA) || defined(PMEM)
        	char filname[100];
                sprintf(filname, "/mnt/pmem0/data/%d%s", size, name);
                mem = pmem_map_file(filname, size, PMEM_FILE_CREATE, 0700, 0, 0);
                assert(mem);
	#ifdef NOVA
		if(create_snapshot <= 0) /* Check that snapshot create has been mapped... */
			create_snapshot = open("/proc/fs/NOVA/pmem0/create_snapshot", O_RDWR);	
		assert(create_snapshot > 0);
	#endif
        #elif defined(PMO)
//                sprintf(filname, "%d%s", size, name);
		printf("Trying to allocate %s\n", name);
                if(!pmo_exists(name))
                        pmo_create(name, size, "DEFAULT\0");

                mem = attach(name, 'w', "DEFAULT\0");
		printf("Attached mem\n");
                assert(mem);
        #elif defined(DRAM)
                mem = malloc(size);
                assert(mem);
        #else
		printf("Did not define what type of allocation should occur.\n");
                assert(0);
        #endif
                return mem;
}

void dealloc_memory(void *ptr, size_t size)
{
	#if defined(NOVA) || defined(PMEM)
		pmem_unmap(ptr, size);
	#elif defined(PMO)
		detach(ptr);
	#elif defined(DRAM)
		free(ptr);
	#endif
		return;
}
