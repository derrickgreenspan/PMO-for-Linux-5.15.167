/* Copyright Derrick Greenspan and the University of Central Florida
 * All rights reserved (c) 2021 */

/* WARNING: THIS PROGRAM HAS THE CAPABILITY TO IRREVOCABLY DESTROY DATA. 
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "pmo_structures.h"


#define PMO_MAX_CONTAINERS 63
#define PMO_MAX_ITEMS 4095

struct sha256_pages *initialize_sha256_list(void);
int verify_pmem_zeroed(void *database, uint64_t start, uint64_t end);
void print_all_items_in_nodelist(void *database);
__u64 find_leaf_by_name(char *name, void *database);
char *get_dax_num(char *argument);
uint64_t get_pmem_size(char *argument, uint64_t *pmo_start,
		uint64_t *pmo_end);
int get_pmem_device(char * argument);
void print_banner(char *version, char *date);
void *map_pmem_device(char * argument, uint64_t *start, uint64_t *end, char is_dax);
int initialize_database_header(void *database, 
		char *name, 
		uint64_t start,
		uint64_t end);

int write_database(void *database, char *name, uint64_t start, uint64_t end);
int zero_pmem(void *database, uint64_t start, uint64_t end);
pmo_nodelist *initialize_nodelist(void);

int main(int argc, char *argv[])
{
	printf("PMO leaf is %d\n", sizeof(struct pmo_entry));
	assert(sizeof(struct pmo_entry) == 64);
	assert(sizeof(pmo_nodelist) == 0x80000000);
	printf("pmo_nodelist is %x\n", sizeof(struct pmo_nodelist_s));
	printf("%x\n", MAX_NODES);
	void *mapping;
	uint64_t start = 0, end = 0;
	print_banner("0.4pe -- per-page encryption","November 3, 2022");

	if(getuid())
	{
		printf("This program must be run as root\n");
		return -EACCES;
	}

	if(argc < 2)
	{
		printf("No device specified.\n"
			"mkpmo expects a device name "
			"(i.e., /dev/dax0.0) "
			"as its first argument.\n");

		printf("Try 'mkpmo -- help' for more information.\n");
		return -ENXIO;
	}

	if(argc < 3)
	{
		printf(	"No database name specified.\n"
			"mkpmo expects a database name "
			"as its second argument.\n");

		printf(	"Try 'mkpmo -- help' for more information.\n");

		return 	-ENXIO;
	}

	if(!strstr(argv[1], "/dev/"))
	{
		printf("The specified device \"%s\" does not exist.\n",
				argv[1]);
		return -ENODEV;
	}

	/*
	if(!strstr(argv[1], "dax"))
	{
		printf("Device must be a DAX char device, "
				"and \"%s\" is not such a device.\n",
				argv[1]);
		return -ENOSTR;
	}
	*/

	if(strlen(argv[2]) > 15)
	{
		printf("The name \"%s\" is %d characters long.\n"
		       "The pmo database must be no longer than "
		       "15 characters.\n",
		       argv[2], strlen(argv[2]));
		return -ENAMETOOLONG;
	}
	
	printf(strstr(argv[1], "dax") != NULL ? 
	"This is a dax volume\n":
	"This is not a dax volume\n");
	mapping = map_pmem_device(argv[1], &start, &end, strstr(argv[1], "dax") != NULL);
	//zero_pmem(mapping, start, end);
	write_database(mapping, argv[2], start, end);
	print_all_items_in_nodelist(mapping);

	assert(mapping);


}

int get_pmem_device(char * argument)
{
	int ifp = open(argument, O_RDWR);
	if(!ifp)
	{
		if(errno == ENOENT)
			printf("No such file \"%s\" exists.\n",
					argument);
		/* RIP */
		exit(-errno);
	}
	return ifp;
}

uint64_t get_pmem_size(char *argument, uint64_t *pmo_start,
		uint64_t *pmo_end)
{
	FILE * ifp;
	char *iomem, *substr;

	char start_string[20], 
	     end_string[20],
	     do_end = 0;

	uint64_t total_size, start,
		 end, lineptr;
		 printf("Getting size for %s\n", argument);

	int i, 
	    start_count = 0,
	    end_count = 0;

	iomem = calloc(sizeof(char), 50);

	substr = get_dax_num(argument);
	
	memset(start_string, '\0', 20);
	memset(end_string, '\0', 20);
	ifp = fopen("/proc/iomem", "r");
	if(!ifp)
	{
		printf("/proc/iomem does not exist!\n");
		exit(-errno);
	}
	while(!feof(ifp))
	{
		getline(&iomem, &lineptr, ifp);
		if(strstr(iomem, substr))
		{
			printf("got %s\n", iomem);
			for(i = 0; i < lineptr; i++)
			{
				if(isxdigit(iomem[i]))
				{
					if(!do_end)
					{
						start_string[start_count] = iomem[i];
						start_count++;
					}
					else
					{
						end_string[end_count] = iomem[i];
						end_count++;
					}
					
				}
				else if(iomem[i] == '-')
					do_end = 1;
				else if(iomem[i] == ':')
					break;
			}
		}
	}
	end = strtoull(end_string, NULL, 16);
	start = strtoull(start_string, NULL, 16);
	total_size = end - start + 1;

	printf("TOTAL SIZE %d\n", total_size);
	*pmo_start = start;
	*pmo_end = end;

	return total_size;
}

void *map_pmem_device(char * argument, uint64_t *start, uint64_t *end,
		char is_dax)
{
	int ifp;
       	void *mapping;
	uint64_t pmo_start, pmo_end;
	uint64_t pmem_size = is_dax ? 
		get_pmem_size(argument, &pmo_start, &pmo_end) :
		476.9*1024*1024*1000;

	if (!is_dax) {
		pmo_start = *start + pmem_size;
		pmo_end = *end + *start + pmem_size;
	}

	ifp = get_pmem_device(argument);

	assert(ifp); 

	mapping = mmap(NULL, pmem_size, PROT_READ|PROT_WRITE,
			is_dax ? MAP_SHARED_VALIDATE | MAP_SYNC : MAP_SHARED,
			ifp, 0);

	printf("Mapped %s, address %X: %s\n", argument, mapping, strerror(errno));

	*start = pmo_start;
	*end = pmo_end;
	printf("Start/End:\t%llx/%llx\n",
			*start, *end);

	
	return mapping;
}

void print_banner(char *version, char *date)
{
	printf("mkpmo %s (%s)\n",
			version, date);
}

char * get_dax_num(char *argument)
{
	char *dax_num = calloc(sizeof(char), 10);
	int i;
	for(i = 5; i < strlen(argument); i++)
	{
		dax_num[i-5] = argument[i];
	}
	printf("Dax device is %s\n", dax_num);	
	return dax_num;
}

int initialize_database_header(void *database, 
		char *name, 
		uint64_t start,
		uint64_t end)
{
	int is_err, i;
	__u64 leaf_to_find;
	pmo_nodelist *nodelist;
	struct sha256_pages *pages = initialize_sha256_list();
	pmo_database *pmo_header = calloc(sizeof(pmo_database), 1);


	nodelist = initialize_nodelist(); /* offset is immediately at the end of the pmo_database */


	assert(pmo_header);
	strcpy(pmo_header->this.header, "PMO\0");

	strcpy(pmo_header->this.name, name);
	pmo_header->this.pmo_range.start = start;
	pmo_header->this.pmo_range.end = end;
	pmo_header->this.nodelist_location = sizeof(pmo_database);
	pmo_header->this.sha256_region_location = sizeof(pmo_database) + 
		sizeof(pmo_nodelist);
	printf("%lld\n", sizeof(struct sha256_pages));
	assert(sizeof(struct sha256_pages) == 2147483648);

	/*  The next free PMO is initially the 
	 *  size of the database + sizeof region_location + the size of the nodelist */
	pmo_header->this.next_free_pmo =  sizeof(pmo_database) + 
		sizeof(pmo_nodelist) + sizeof(struct sha256_pages);

	/* PMO region location begins here */
	pmo_header->this.pmo_region_location = pmo_header->this.next_free_pmo;

	printf("Next free PMO is %llX\n", pmo_header->this.next_free_pmo);
	printf("Nodelist Location is: %llX\n", pmo_header->this.nodelist_location);
	printf("SHA256 Region Location is: %llX\n", pmo_header->this.sha256_region_location);
	memcpy(database, pmo_header, sizeof(pmo_database));
	memcpy(database + sizeof(pmo_database), nodelist, sizeof(pmo_nodelist));
	memcpy(database + sizeof(pmo_database) + sizeof(pmo_nodelist), pages,
			sizeof(struct sha256_pages));

	if(memcmp(database, pmo_header, sizeof(pmo_database)) != 0 || 
			memcmp(database + sizeof(pmo_database), nodelist, sizeof(pmo_nodelist)) != 0)
	{
		printf("Failed to write pmo header data!\n");
		printf("Debug information begins below:\n");
		printf("Name: %s\n", ((struct pmo_database_s *)database)->name);
		printf("Range: \n\tStart:%llx\n\tEnd:%llx\n",
		       	((struct pmo_database_s *)database)->pmo_range.start,
		       	((struct pmo_database_s *)database)->pmo_range.end);

		exit(-1);
	}

	printf("Database header has been written with the following information:\n");
	printf("--------\n");
	printf("Name: %s\n", ((struct pmo_database_s *)database)->name);
	printf("Range: \n\tStart:%lx\n\tEnd:%lx\n",
		       	((struct pmo_database_s *)database)->pmo_range.start,
		       	((struct pmo_database_s *)database)->pmo_range.end);

	printf("Nodelist location: %lx\n",
			((struct pmo_database_s *)database)->nodelist_location);
	printf("Sha256 region location: %lx\n",
			((struct pmo_database_s *)database)->sha256_region_location);
	printf("Next Free PMO: %lx\n", 
			((struct pmo_database_s *)database)->next_free_pmo);

	

}

int write_database(void *database, char *name, uint64_t start, uint64_t end)
{
	printf("Writing the database header\n");
	initialize_database_header(database, name, start, end);
}

struct sha256_pages *initialize_sha256_list(void)
{
	char sanity[32];
	memset(sanity, 0xFF, 32);
	printf("Sanity: %llX, %llX, %llX\n",
			sanity[0], sanity[1], sanity[2]);
	int i;
	 struct sha256_pages *sha256_list = calloc(sizeof(struct sha256_pages), 1);
	
	 for(i = 0; i < (0x4000000); i++) {
	 	memcpy(sha256_list->page_hash[i].sum, sanity, 32);
		assert(memcmp(sanity, sha256_list->page_hash[i].sum, 32) == 0);
	 }
	 return sha256_list;
}

pmo_nodelist *initialize_nodelist()
{
	int i;
	/* Sanity checks */
	assert(sizeof(struct pmo_entry) == 64);

	pmo_nodelist *nodelist = 
		calloc(sizeof(pmo_nodelist), 1); 

	//nodelist->this.nodes = calloc(sizeof(struct pmo_entry),MAX_NODES);

	/*  Sanity check */
	if(nodelist->this.nodes[0].name[0] != 0)
	{
		printf("PMO nodelist failed to be created!\n");
		return NULL;
	}


	for(i = 0; i < MAX_NODES; i++)
	{
		nodelist->this.nodes[i].pm_primary = UINT32_MAX;
		nodelist->this.nodes[i].state = 0; 
	}

	return nodelist;	
}

__u64 find_leaf_by_name(char *name, void *database)
{
	struct pmo_entry *node_to_interrogate;
	int i = 0;
	node_to_interrogate = 
		&((pmo_nodelist *)(database +
		sizeof(pmo_database)))->this.nodes[djb2_hash(name) 
		% MAX_NODES];

	if(strcmp(name, node_to_interrogate->name) == 0)
		return djb2_hash(name) % MAX_NODES;

	else if(node_to_interrogate->pm_primary != UINT32_MAX)
	{
		printf("WARNING: HASH COLLISIONS ARE NOT HANDLED RIGHT NOW\n");
		exit -1;
	}

	return UINT32_MAX;
}

void print_all_items_in_nodelist(void *database)
{
	struct pmo_entry *node_to_interrogate;
	int i = 0;

	printf("\n\nLIST OF NODES IN PMO DATABASE \"%s\"\n",
			((pmo_database *)(database))->this.name);
	printf("\nPMO DATABASE STARTS AT 0x%llX, ENDS AT 0x%llX, %lld TOTAL NODES",
			((pmo_database *)(database))->this.pmo_range.start,
			((pmo_database *)(database))->this.pmo_range.end,
			((pmo_nodelist *)(database + sizeof(pmo_database)))->this.allocated_nodes);
	assert(sizeof(pmo_database) == (((pmo_database *)(database))->this.nodelist_location));
	printf("\nNODE #\t\t\t\tNAME\t\tADDR\n");
	while(i < MAX_NODES)
	{
		/*  node_to_interrogate = the starting address of the leaves 
		 * + 8 bytes for the 64 bit integer which specifies 
		 * the number of allocated nodes */
		node_to_interrogate = (struct pmo_entry *)
				((database + sizeof(pmo_database)
				       	+ sizeof(__u64) + 
					(sizeof(struct pmo_entry))*i)); 

		if(node_to_interrogate->pm_primary != UINT32_MAX)
			printf("%llx: %lld\t\t%s\t\t0x%llX\n", node_to_interrogate,
				       	i, node_to_interrogate->name,
					node_to_interrogate->pm_primary);
		i++;
	}
}

/* Wipe all the data in the pmem */
int zero_pmem(void *database, uint64_t start, uint64_t end)
{
	uint64_t i, 
		 pmem_range = end - start;		

	printf("Zeroing all addresseses in range 0x%llX-0x%llX. 0x%llX wide...\n\n",
			start, end, pmem_range);
	fflush(stdout);

	memset(database, 0, pmem_range);
	return 0;

	for(i = 0; i < pmem_range; i++)
	{
		if((i % 0x10000) == 0x0)
		{
			printf("\rReading Address 0x%llX / 0x%llX: %.2f%%",
					i, pmem_range,
					(float)(((float) i / (float)pmem_range)*100));
		}

		if(((char *)(database))[i] != 0) /* Only zero the cell if needed */
		{
			printf("\rZeroing Address 0x%llX / 0x%llX: %.2f%%",
					i, pmem_range,
					(float)(((float) i / (float)pmem_range)*100));
			((char *)(database))[i] = 0;
		}

	}
	printf("\nDone! All addresses should be zeroed now.\n");


	/* A slow sanity check to ensure that the NVMM doesn't 
	 * have any stuck data */
	return  0; //verify_pmem_zeroed(database, start, end); 
}

int verify_pmem_zeroed(void *database, uint64_t start, uint64_t end)
{
	uint64_t i,
		 pmem_range = end - start;		
	printf("Verifying that all addresses were zeroed...\n");
	fflush(stdout);

	for(i = 0; i < (end - start); i++)
	{
		if(i % 10000 == 0)
		{
			printf("\rAddress 0x%llX / 0x%llX: %.2f%%",
					i, pmem_range,
					(float)(((float) i / (float)pmem_range)*100));
		}
		if(((char *)database)[i] != 0)
		{
			printf("An address was not zero! It must have been stuck\n");
			return 0;
		}
	}
	printf("All addresses were zeroed successfully!\n");

	return 1;
}
