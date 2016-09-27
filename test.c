#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include "src/jitas.h"

typedef intptr_t (*asmfunc)(int argc, char **argv);

void *symbolresolve_dlfcn(const char *symbol, void *data)
{
	return dlsym(NULL, symbol);
}

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
	jitas_context_t ctx;
	ctx.ptr = buff;
	ctx.resolver = symbolresolve_dlfcn;
	int len = jitas_assemble(&ctx, str);
	bool linkSuccess = jitas_link(&ctx, NULL);

	//output all assembly errors
	for(;;)
	{
		int line;
		char *err = jitas_error(&line);
		if(err == NULL)
			break;

		fprintf(stderr, "line %d: %s\n", line, err);
		free(err);
	}

	//if jitas_assemble returns 0 when there was an error
	if(!linkSuccess || len == 0)
		return 1;

	for(int i = 0; i < len; i++)
		printf("%02hhX ", buff[i]);
	printf("\n");

	//call the assembled instructions like a function
	asmfunc func = (void *)buff;
	return func(argc, argv);
}
