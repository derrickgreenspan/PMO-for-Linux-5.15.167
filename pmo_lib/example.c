#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "pmo.h"
int main(int argc, char *argv[])
{
	char *ptr, *ptr1, name[32], key[16];
	size_t size;
	/* Verify data are still there */

	if(argc < 5) {
		printf("Error: Expected 4 arguments, but got %d arguments\n",
				argc);
		printf("Please enter 4 arguments in this form:\n\t");
		printf("name, key, size, message\n");
		exit(-1);
	}
	strcpy(name,argv[1]);
	strcpy(key, argv[2]);
	size = atoi(argv[3]);

	printf("Creating a PMO with name %s, key %s, and size %ld\n",
			name, key, size);
	pmo_create(name, size, key);
	ptr = pmo_attach(name, 'w', key); 

	strcpy(ptr, argv[4]);

	printf("Writing: %s\n", ptr);
	psync(ptr);
	pmo_detach(ptr);
	
	ptr1 = pmo_attach(name, 'w', key);
	printf("You wrote: %s\n", ptr1);
	pmo_detach(ptr1);

	return 0;
}

