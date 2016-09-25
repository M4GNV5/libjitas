#include <string.h>
#include <stdbool.h>

#include "jitas.h"

static int instructionCount;
static jitas_instruction_t instructions[];

bool compareArgs(jitas_argument_t *arg, jitas_argtype_t opArg)
{
	if(arg->type == opArg)
		return true;
	if(arg->type == JITAS_ARG_IMM && opArg == JITAS_ARG_IMM8 && arg->imm >= INT8_MIN && arg->imm <= INT8_MAX)
		return true;

	if(arg->type == JITAS_ARG_REG)
	{
		if(opArg == JITAS_ARG_REGA && arg->mem.base == 0)
			return true;
		if(opArg == JITAS_ARG_REGCL && arg->size == 1 && arg->mem.base == 1)
			return true;
	}

	return false;
}

jitas_instruction_t *jitas_findInstruction(const char *label, jitas_argument_t *src, jitas_argument_t *dst)
{
	for(int i = 0; i < instructionCount; i++)
	{
		if(strcmp(instructions[i].label, label) == 0)
		{
			jitas_instruction_t *ins = instructions + i;
			if(compareArgs(src, ins->source) && compareArgs(dst, ins->destination))
				return ins;
		}
	}

	return NULL;
}

static jitas_instruction_t instructions[] = {
	{"mov", 1, {0x88}, 1, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{"mov", 1, {0x89}, 4, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{"mov", 1, {0x8A}, 1, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{"mov", 1, {0x8B}, 4, JITAS_ARG_MODRM, JITAS_ARG_REG},
	//... seg:offset movs 8C, 8E, A0, A1, A2, A3
	{"mov", 1, {0xB0}, 1, JITAS_ARG_IMM, JITAS_ARG_REG},
	{"mov", 1, {0xB8}, 4, JITAS_ARG_IMM, JITAS_ARG_REG},
	{"mov", 1, {0xC6, 0}, 1, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{"mov", 1, {0xC7, 0}, 4, JITAS_ARG_IMM, JITAS_ARG_MODRM},

	{"add", 1, {0x04}, 1, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{"add", 1, {0x05}, 4, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{"add", 1, {0x80, 0}, 1, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{"add", 1, {0x81, 0}, 4, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{"add", 1, {0x83, 0}, 4, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{"add", 1, {0x00}, 1, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{"add", 1, {0x01}, 4, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{"add", 1, {0x02}, 1, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{"add", 1, {0x03}, 4, JITAS_ARG_MODRM, JITAS_ARG_REG},

	{"sub", 1, {0x2C}, 1, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{"sub", 1, {0x2D}, 4, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{"sub", 1, {0x80, 5}, 1, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{"sub", 1, {0x81, 5}, 4, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{"sub", 1, {0x83, 5}, 4, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{"sub", 1, {0x28}, 1, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{"sub", 1, {0x29}, 4, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{"sub", 1, {0x2A}, 1, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{"sub", 1, {0x2B}, 4, JITAS_ARG_MODRM, JITAS_ARG_REG},

	{"shl", 1, {0xD0, 4}, 1, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"shl", 1, {0xD1, 4}, 4, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"shl", 1, {0xD2, 4}, 1, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{"shl", 1, {0xD3, 4}, 4, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{"shl", 1, {0xC0, 4}, 1, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{"shl", 1, {0xC1, 4}, 4, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"sal", 1, {0xD0, 4}, 1, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"sal", 1, {0xD1, 4}, 4, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"sal", 1, {0xD2, 4}, 1, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{"sal", 1, {0xD3, 4}, 4, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{"sal", 1, {0xC0, 4}, 1, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{"sal", 1, {0xC1, 4}, 4, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"shr", 1, {0xD0, 5}, 1, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"shr", 1, {0xD1, 5}, 4, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"shr", 1, {0xD2, 5}, 1, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{"shr", 1, {0xD3, 5}, 4, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{"shr", 1, {0xC0, 5}, 1, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{"shr", 1, {0xC1, 5}, 4, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"sar", 1, {0xD0, 7}, 1, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"sar", 1, {0xD1, 7}, 4, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"sar", 1, {0xD2, 7}, 1, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{"sar", 1, {0xD3, 7}, 4, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{"sar", 1, {0xC0, 7}, 1, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{"sar", 1, {0xC1, 7}, 4, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"jmp", 1, {0xEB}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jmp", 1, {0xE9}, 4, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jmp", 1, {0xFF, 4}, 4, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	//... far jmp

	{"jo", 1, {0x70}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jno", 1, {0x71}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jb", 1, {0x72}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jc", 1, {0x72}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnae", 1, {0x72}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnb", 1, {0x73}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnc", 1, {0x73}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jae", 1, {0x73}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"je", 1, {0x74}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jz", 1, {0x74}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jne", 1, {0x75}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnz", 1, {0x75}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jbe", 1, {0x76}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jna", 1, {0x76}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnbe", 1, {0x77}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"ja", 1, {0x77}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"js", 1, {0x78}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jns", 1, {0x79}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jp", 1, {0x7A}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jpe", 1, {0x7A}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jpo", 1, {0x7B}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnp", 1, {0x7B}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jl", 1, {0x7C}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnge", 1, {0x7C}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnl", 1, {0x7D}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jge", 1, {0x7D}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jle", 1, {0x7E}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jng", 1, {0x7E}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnle", 1, {0x7F}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jg", 1, {0x7F}, 1, JITAS_ARG_NONE, JITAS_ARG_IMM},

	{"call", 1, {0xE8}, 4, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"call", 1, {0xFF, 2}, 4, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	//... far call

	{"ret", 1, {0xC3}, -1, JITAS_ARG_NONE, JITAS_ARG_NONE},
	{"ret", 1, {0xC2}, 2, JITAS_ARG_NONE, JITAS_ARG_IMM},
	//... far ret

	{"push", 1, {0xFF, 6}, -1, JITAS_ARG_MODRM, JITAS_ARG_NONE},
	{"push", 1, {0x50}, -1, JITAS_ARG_REG, JITAS_ARG_NONE},
	{"push", 1, {0x6A}, 1, JITAS_ARG_IMM, JITAS_ARG_NONE},
	{"push", 1, {0x68}, 4, JITAS_ARG_IMM, JITAS_ARG_NONE},
	//... push segment register

	{"pop", 1, {0x8F, 0}, -1, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{"pop", 1, {0x50}, -1, JITAS_ARG_NONE, JITAS_ARG_REG},
	//... pop segment register
};
static int instructionCount = sizeof(instructions) / sizeof(jitas_instruction_t);
