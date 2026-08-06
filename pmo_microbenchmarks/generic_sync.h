#ifdef NOVA
    #define do_snapshot() dprintf(create_snapshot, "\1");
#else
    #define do_snapshot() {};
#endif

#if defined(NOVA) || defined(PMEM)
        #define sync_memory(ptr, size) pmem_persist(ptr, size); do_snapshot();

#elif defined(PMO)
        #define sync_memory(ptr, size) \
		printf("SYNCING %lX\n", ptr);\
		psync(ptr);

#elif defined(DRAM)
        #define sync_memory(ptr, size) {};

#else
        #define sync_memory(ptr, size) assert(0);

#endif
