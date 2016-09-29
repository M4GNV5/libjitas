#libjitas

##Documentation
```C
//this is the type of the 'resolver' callback within the 'context' object
//if a symbol cannot be resolved it shall return NULL
typedef void *(*jitas_symbolresolver_t)(const char *symbol, void *data);

//this is the conext struct passed to all functions
//you have to set 'ptr' to a pointer where the assembled instructions will be written
//and 'resolver' to a function that will be used by jitas_link to resolve symbols
typedef struct
{
	int line;
	uint8_t *ptr;
	struct errorlist *firstError;
	struct errorlist *lastError;
	jitas_symboltable_t *symbols;
	jitas_symboltable_t *localSymbols;
	jitas_symbolresolver_t resolver;
} jitas_context_t;

//assembles a assembly code string
//returns the count of bytes written to ctx->ptr
//'ctx' is a context object ('ptr' should be set by you)
//'str' is the assembly source code string
int jitas_assemble(jitas_context_t *ctx, const char *str);

//links symbol references in assembled instructions
//returns true if all symbols could be resolved, false otherwise
//'ctx' is the context object passed to 'jitas_assemble' ('resolver' should be set by you)
//'data' will be passed to the resolver
bool jitas_link(jitas_context_t *ctx, void *data);

//assemble and link errors can be retrieved with this function
//you probably want to call this in a loop to clear out all errors (as there can be multiple)
//'ctx' is the context object used before with 'jitas_assemble' and/or 'jitas_link'
//'line' is a pointer to an int that will be set to the line the error occured on
char *jitas_error(jitas_context_t *ctx, int *line);

//this can be used to retrieve the address of a symbol defined in assembly code
//if the symbol is not found, NULL is returned, otherwise the address of the symbol
//'ctx' is the context passed to 'jitas_assemble'
//'label' is the name of the label
uint8_t *jitas_findLocalSymbol(jitas_context_t *ctx, const char *label);
```

##Example
This example assembles `mov %rax, 42(%rbx)` and hexdumps the assembled instruction to stdout.
For a more complex example (that is basically a assembly interpreter) check [test.c](test.c)
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
		int line;
		char *err = jitas_error(&line);
		if(err == NULL)
			break;

		fprintf(stderr, "line %d: %s\n", line, err);
		free(err);
	}

	//hexdump the assembled instruction to stdout
	for(int i = 0; i < len; i++)
	{
		printf("%02hhX ", buff[i]);
	}
	printf("\n");
	return 0;
}
```
