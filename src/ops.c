#include <string.h>

#include "jitas.h"

bool compareArgs(jitas_argument_t *arg, jitas_argtype_t opArg, jitas_instruction_t *ins)
{
	if(arg->type == JITAS_ARG_NONE && opArg == JITAS_ARG_NONE)
		return true;

	if(arg->type == JITAS_ARG_REG)
	{
		if(opArg == JITAS_ARG_REGA && arg->mem.base == 0)
			return true;
		if(opArg == JITAS_ARG_REGCL && arg->size == 1 && arg->mem.base == 1)
			return true;
	}

	if(arg->type != JITAS_ARG_IMM && ((arg->size != 1 && ins->isByteOp) || (arg->size == 1 && !ins->isByteOp)))
		return false;

	if(arg->type == opArg)
		return true;

	if(arg->type == JITAS_ARG_IMM)
	{
		if(opArg == JITAS_ARG_IMM8 && arg->imm >= INT8_MIN && arg->imm <= INT8_MAX)
			return true;
		if(opArg == JITAS_ARG_IMM16 && arg->imm >= INT16_MIN && arg->imm <= INT16_MAX)
			return true;
		if(opArg == JITAS_ARG_IMM_MAX32 && arg->imm >= INT32_MIN && arg->imm <= INT32_MAX)
			return true;
	}

	if(arg->type == JITAS_ARG_REG && opArg == JITAS_ARG_MODRM)
			return true;

	return false;
}

jitas_instruction_t *jitas_findInstruction(const char *label, jitas_argument_t *src, jitas_argument_t *dst)
{
	for(int i = 0; i < jitas_instructionCount;)
	{
		if(strcmp(jitas_instructions[i].label, label) == 0)
		{
			do
			{
				jitas_instruction_t *ins = jitas_instructions + i;
				if(compareArgs(src, ins->source, ins) && compareArgs(dst, ins->destination, ins))
					return ins;

				i++;
			} while(jitas_instructions[i].label == NULL);

			return NULL;
		}
		else
		{
			do
			{
				i++;
			} while(jitas_instructions[i].label == NULL);
		}
	}

	return NULL;
}

jitas_instruction_t jitas_instructions[] = {
	{"mov", 1, {0x88}, true, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x89}, false, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x8A}, true, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{NULL, 1, {0x8B}, false, JITAS_ARG_MODRM, JITAS_ARG_REG},
	//... seg:offset movs 8C, 8E, A0, A1, A2, A3
	{NULL, 1, {0xB0}, true, JITAS_ARG_IMM, JITAS_ARG_REG},
	{NULL, 1, {0xB8}, false, JITAS_ARG_IMM, JITAS_ARG_REG},
	{NULL, 1, {0xC6, 0}, true, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{NULL, 1, {0xC7, 0}, false, JITAS_ARG_IMM, JITAS_ARG_MODRM},

	{"add", 1, {0x04}, true, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{NULL, 1, {0x05}, false, JITAS_ARG_IMM_MAX32, JITAS_ARG_REGA},
	{NULL, 1, {0x80, 0}, true, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{NULL, 1, {0x81, 0}, false, JITAS_ARG_IMM_MAX32, JITAS_ARG_MODRM},
	{NULL, 1, {0x83, 0}, false, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0x00}, true, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x01}, false, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x02}, true, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{NULL, 1, {0x03}, false, JITAS_ARG_MODRM, JITAS_ARG_REG},

	{"sub", 1, {0x2C}, true, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{NULL, 1, {0x2D}, false, JITAS_ARG_IMM_MAX32, JITAS_ARG_REGA},
	{NULL, 1, {0x80, 5}, true, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{NULL, 1, {0x81, 5}, false, JITAS_ARG_IMM_MAX32, JITAS_ARG_MODRM},
	{NULL, 1, {0x83, 5}, false, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0x28}, true, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x29}, false, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x2A}, true, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{NULL, 1, {0x2B}, false, JITAS_ARG_MODRM, JITAS_ARG_REG},

	{"shl", 1, {0xD0, 4}, true, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD1, 4}, false, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD2, 4}, true, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xD3, 4}, false, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xC0, 4}, true, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0xC1, 4}, false, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"sal", 1, {0xD0, 4}, true, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD1, 4}, false, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD2, 4}, true, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xD3, 4}, false, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xC0, 4}, true, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0xC1, 4}, false, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"shr", 1, {0xD0, 5}, true, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD1, 5}, false, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD2, 5}, true, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xD3, 5}, false, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xC0, 5}, true, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0xC1, 5}, false, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"sar", 1, {0xD0, 7}, true, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD1, 7}, false, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD2, 7}, true, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xD3, 7}, false, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xC0, 7}, true, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0xC1, 7}, false, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"jmp", 1, {0xEB}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{NULL, 1, {0xE9}, false, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{NULL, 1, {0xFF, 4}, false, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	//... far jmp

	{"jo", 1, {0x70}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jno", 1, {0x71}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jb", 1, {0x72}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jc", 1, {0x72}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnae", 1, {0x72}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnb", 1, {0x73}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnc", 1, {0x73}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jae", 1, {0x73}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"je", 1, {0x74}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jz", 1, {0x74}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jne", 1, {0x75}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnz", 1, {0x75}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jbe", 1, {0x76}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jna", 1, {0x76}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnbe", 1, {0x77}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"ja", 1, {0x77}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"js", 1, {0x78}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jns", 1, {0x79}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jp", 1, {0x7A}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jpe", 1, {0x7A}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jpo", 1, {0x7B}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnp", 1, {0x7B}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jl", 1, {0x7C}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnge", 1, {0x7C}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnl", 1, {0x7D}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jge", 1, {0x7D}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jle", 1, {0x7E}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jng", 1, {0x7E}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jnle", 1, {0x7F}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{"jg", 1, {0x7F}, true, JITAS_ARG_NONE, JITAS_ARG_IMM},

	{"call", 1, {0xE8}, false, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{NULL, 1, {0xFF, 2}, false, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	//... far call

	{"ret", 1, {0xC3}, false, JITAS_ARG_NONE, JITAS_ARG_NONE},
	{NULL, 1, {0xC2}, false, JITAS_ARG_NONE, JITAS_ARG_IMM16},
	//... far ret

	{"push", 1, {0xFF, 6}, false, JITAS_ARG_MODRM, JITAS_ARG_NONE},
	{NULL, 1, {0x50}, false, JITAS_ARG_REG, JITAS_ARG_NONE},
	{NULL, 1, {0x6A}, true, JITAS_ARG_IMM, JITAS_ARG_NONE},
	{NULL, 1, {0x68}, false, JITAS_ARG_IMM_MAX32, JITAS_ARG_NONE},
	//... push segment register

	{"pop", 1, {0x8F, 0}, false, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0x50}, false, JITAS_ARG_NONE, JITAS_ARG_REG},
	//... pop segment register
};
int jitas_instructionCount = sizeof(jitas_instructions) / sizeof(jitas_instruction_t);
