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

	//special
	JITAS_ARG_IMM8, //imm8 on a 2/4/8 byte instruction
	JITAS_ARG_REGA, //al, ax, eax, rax
	JITAS_ARG_REGCL, //cl
} jitas_argtype_t;

typedef struct
{
	jitas_argtype_t type;
	int8_t size;
	bool needsRex;
	union
	{
		int64_t imm;
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
	int8_t size;
	jitas_argtype_t source;
	jitas_argtype_t destination;
} jitas_instruction_t;

jitas_instruction_t *jitas_findInstruction(const char *label, jitas_argument_t *src, jitas_argument_t *dst);
bool jitas_findRegister(const char *label, int8_t *size, uint8_t *id, bool *needsRex);
int jitas_encode(uint8_t *ptr, jitas_instruction_t *ins, jitas_argument_t *src, jitas_argument_t *dst);

#endif
