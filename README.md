#libjitas

##Functions
```C
//ptr is a pointer where to put the assembled instructions
//	(you probably want this to be executable memory)
//str is the assembly source code
//returns the amount of bytes written to ptr
int jitas_assemble(uint8_t *ptr, const char *str);

//assemble errors can be retrieved with this function
//you probably want to call this in a loop to clear out all errors (as there can be multiple)
char *jitas_error();
```

##Short Example
This example assembles `mov %rax, 42(%rbx)` and hexdumps the assembled instruction to stdout
```C
int main()
{
	//the assembled instruction will be put into this buffer
	uint8_t buff[32] = {0};

	//assemble the instruction
	int len = jitas_assemble(buff, "mov %rax, 42(%rbx)");

	//print any errors
	for(;;)
	{
		char *err = jitas_error();
		if(err == NULL)
			break;

		fprintf(stderr, "%s\n", err);
		free(err);
	}

	//if jitas_assemble returns 0 there was an error
	if(len == 0)
		return 1;

	//hexdump the assembled instruction to stdout
	for(int i = 0; i < len; i++)
	{
		printf("%02hhX ", buff[i]);
	}
	printf("\n");
	return 0;
}
```

##Long Example
This example assembles code from a file, puts the bytecode into a executable
mapped memory region and calls it (putting argc into `%rdi` and argv into `%rsi`)
```C
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "src/jitas.h"

typedef intptr_t (*asmfunc)(int argc, char **argv);

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return 1;
	}

	//open file
	FILE *fd = fopen(argv[1], "rb");

	if(fd == NULL)
	{
		fprintf(stderr, "Invalid file %s\n", argv[1]);
		return 1;
	}

	//get file size
	fseek(fd, 0, SEEK_END);
	long fsize = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	//read file into buffer
	char *str = malloc(fsize + 1);
	fread(str, fsize, 1, fd);
	fclose(fd);

	str[fsize] = 0;

	//allocate a executable memory region
	uint8_t *buff = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	//assemble the assembly code from the file into said memory region
	int len = jitas_assemble(buff, str);

	//output all assembly errors
	for(;;)
	{
		char *err = jitas_error();
		if(err == NULL)
			break;

		fprintf(stderr, "%s\n", err);
		free(err);
	}

	//if jitas_assemble returns 0 there was an error
	if(len == 0)
		return 1;

	//call the assembled instructions like a function
	asmfunc func = (void *)buff;
	return func(argc, argv);
}
```
