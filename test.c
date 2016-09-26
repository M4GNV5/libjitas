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

	for(int i = 0; i < len; i++)
		printf("%02hhX ", buff[i]);
	printf("\n");

	asmfunc func = (void *)buff;
	return func(argc, argv);
}
