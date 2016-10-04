#libjitas
Library capable of assembling a source of assembly code in GNU syntax at runtime
writing the assembled instructions to a pointer provided.

##Documentation
```C
typedef void *(*jitas_symbolresolver_t)(const char *symbol, void *data);
```
This is the type of the `resolver` callback within the `context` object
- if a symbol cannot be resolved it shall return NULL
- `symbol` is the 0 terminated name of the symbol
- `data` is the pointer passed to `jitas_link`

```C
void jitas_init(jitas_context_t *ctx, uint8_t *ptr, jitas_symbolresolver_t resolver);
```
Initializes a `jitas_context_t` struct. You can also pass NULL for `ptr` and/or
`resolver` and set them manually later
- `ctx` is the context to initialize
- `ptr` is a pointer to (executable?!) memory
- `resolver` is a callback called by jitas_link

```C
int jitas_assemble(jitas_context_t *ctx, const char *str);
```
Assembles a assembly code string, returns the count of bytes written to ctx->ptr
- `ctx` is the context object passed to `jitas_init`
- `str` is the assembly source code string

```C
bool jitas_link(jitas_context_t *ctx, void *data);
```
Links symbol references in assembled instructions.
Returns true if all symbols could be resolved, false otherwise
- `ctx` is the context object passed to `jitas_init` and `jitas_assemble`
- `data` will be passed to the resolver

```C
char *jitas_error(jitas_context_t *ctx, int *line);
```
Assemble and link errors can be retrieved with this function.
You probably want to call this in a loop to clear out all errors (as there can be multiple)
- `ctx` is the context object used before with `jitas_assemble` and/or `jitas_link`
- `line` is a pointer to an int that will be set to the line the error occured on

```C
uint8_t *jitas_findLocalSymbol(jitas_context_t *ctx, const char *label);
```
This can be used to retrieve the address of a symbol defined in assembly code.
If the symbol is not found, NULL is returned, otherwise the address of the symbol
- `ctx` is the context passed to `jitas_assemble`
- `label` is the name of the label



```C
typedef struct jitas_context
{
	int line;
	uint8_t *ptr;
	struct errorlist *firstError;
	struct errorlist *lastError;
	jitas_symboltable_t *symbols;
	jitas_symboltable_t *localSymbols;
	jitas_symbolresolver_t resolver;
} jitas_context_t;
```
This is the conext struct passed to all functions.
If you passed NULL for `ptr` or `resolver` to `jitas_init` you should set `context.ptr`
or `context.resolver` before calling `jitas_assemble` or `jitas_link` respectively

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
