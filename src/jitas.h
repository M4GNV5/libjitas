#ifndef _JITAS_H
#define _JITAS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
	//common
	JITAS_ARG_NONE,
	JITAS_ARG_REG, //without modrm: reg id is in the last 3 bits of opcode
	JITAS_ARG_IMM, //size defined by arg.size
	JITAS_ARG_MODRM, //modrm byte + optional sib byte + optional disp8/disp32
	JITAS_ARG_SYMBOL, //symbol (assembled as either rel8, rel32 or modrm)
	JITAS_ARG_SYMBOL_ADDRESS, //symbol address (assembled as imm64)

	//special
	JITAS_ARG_IMM8, //imm8 on a 2/4/8 byte instruction
	JITAS_ARG_IMM16, //imm16 on a 1/4/8 byte instruction
	JITAS_ARG_IMM_MAX32, //imm size defined by arg.size but max 4 byte
	JITAS_ARG_REL8, //relative offset to symbol in imm8 form
	JITAS_ARG_REL32, //relative offset to symbol in imm32 form
	JITAS_ARG_REGA, //al, ax, eax, rax
	JITAS_ARG_REGCL, //cl
} jitas_argtype_t;

typedef enum
{
	JITAS_SIZE_IGNORE, //no specal behaviour for the arguments necessary
	JITAS_SIZE_BYTE, //fixed 1 byte operation
	JITAS_SIZE_PTR, //fixed sizeof(void *) byte operation
	JITAS_SIZE_ANY, //2/4/8 byte operation (depending on suffix)
} jitas_size_t;

typedef struct
{
	jitas_argtype_t type;
	int8_t size;
	bool needsRex;
	union
	{
		int64_t imm;
		const char *symbol;
		//uint8_t reg; use mem.base instead
		struct
		{
			uint8_t base;
			uint8_t index;
			uint8_t scale;
			int32_t offset;
		} mem;
	};
} jitas_argument_t;

typedef struct
{
	const char *label;
	int oplen;
	uint8_t opcode[3];
	jitas_size_t size;
	jitas_argtype_t source;
	jitas_argtype_t destination;
	int64_t flags;
} jitas_instruction_t;

typedef struct jitas_symboltable
{
	int size;
	int line;
	const char *symbol;
	uint8_t *nextInsPtr;
	uint8_t *ptr;
	struct jitas_symboltable *next;
} jitas_symboltable_t;

typedef void *(*jitas_symbolresolver_t)(const char *symbol, void *data);

typedef struct jitas_context
{
	int line;
	uint8_t *ptr;
	const char *identifierToken;
	struct errorlist *firstError;
	struct errorlist *lastError;
	jitas_symboltable_t *symbols;
	jitas_symboltable_t *localSymbols;
	jitas_symbolresolver_t resolver;
} jitas_context_t;

extern int jitas_instructionCount;
extern jitas_instruction_t jitas_instructions[];

jitas_instruction_t *jitas_findUnsizedInstruction(jitas_context_t *ctx, char *buff, jitas_argument_t *src, jitas_argument_t *dst);
jitas_instruction_t *jitas_findInstruction(const char *label, jitas_argument_t *src, jitas_argument_t *dst, bool *isKnown);
char *jitas_findInstructionError(const char *label, jitas_argument_t *src, jitas_argument_t *dst);
bool jitas_findRegister(const char *label, int8_t *size, uint8_t *id, bool *needsRex);
bool jitas_findRegisterArg(const char *label, jitas_argument_t *arg);
void jitas_addError(jitas_context_t *ctx, char *msg, int line);
void jitas_encode(jitas_context_t *ctx, jitas_instruction_t *ins, jitas_argument_t *src, jitas_argument_t *dst);
bool jitas_parse(const char **str, jitas_context_t *ctx, char *buff, jitas_argument_t *src, jitas_argument_t *dst);

void jitas_init(jitas_context_t *ctx, uint8_t *ptr, jitas_symbolresolver_t resolver);
uint8_t *jitas_findLocalSymbol(jitas_context_t *ctx, const char *label);
int jitas_assemble(jitas_context_t *ctx, const char *str);
bool jitas_link(jitas_context_t *ctx, void *data);
char *jitas_error(jitas_context_t *ctx, int *line);

#endif
