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
	FILE *fd = fopen(argv[1], "r");

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

	//assemble and link the code from the file into 'buff'
	jitas_context_t ctx;
	jitas_init(&ctx, buff, symbolresolve_dlfcn);
	int len = jitas_assemble(&ctx, str);
	bool hadError = !jitas_link(&ctx, NULL);

	//output all assemble & link errors
	for(;;)
	{
		int line;
		char *err = jitas_error(&ctx, &line);
		if(err == NULL)
			break;

		hadError = true;
		fprintf(stderr, "line %d: %s\n", line, err);
		free(err);
	}

	//abort if there was an error
	if(hadError)
		return 1;

	//hexdump the assembled instructions
	for(int i = 0; i < len; i++)
		printf("%02hhX ", buff[i]);
	printf("\n");

	//call the assembled instructions like a function
	asmfunc func = (void *)buff;
	return func(argc, argv);
}
