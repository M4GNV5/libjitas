#include <string.h>

#include "jitas.h"

bool compareArgs(jitas_argument_t *arg, jitas_argtype_t opArg, jitas_instruction_t *ins)
{
	if(arg->type == JITAS_ARG_NONE && opArg == JITAS_ARG_NONE)
		return true;

	if(arg->type == JITAS_ARG_REG && opArg == JITAS_ARG_REGCL && arg->size == 1 && arg->mem.base == 1)
		return true;

	if(arg->type == JITAS_ARG_SYMBOL)
	{
		if(opArg == JITAS_ARG_REL8)
			return true;
		if(opArg == JITAS_ARG_REL32)
			return true;
		if(opArg == JITAS_ARG_MODRM)
			return true;
	}

	if(arg->type == JITAS_ARG_IMM)
	{
		if(opArg == JITAS_ARG_IMM8 && arg->imm >= INT8_MIN && arg->imm <= INT8_MAX)
			return true;
		if(opArg == JITAS_ARG_IMM16 && arg->imm >= INT16_MIN && arg->imm <= INT16_MAX)
			return true;
		if(opArg == JITAS_ARG_IMM_MAX32 && arg->imm >= INT32_MIN && arg->imm <= INT32_MAX)
			return true;
	}
	else if((ins->size == JITAS_SIZE_BYTE && arg->size != 1)
			|| (ins->size == JITAS_SIZE_ANY && arg->size == 1)
			|| (ins->size == JITAS_SIZE_PTR && arg->size != sizeof(void *)))
	{
		return false;
	}

	if(arg->type == opArg)
		return true;
	if(arg->type == JITAS_ARG_REG && opArg == JITAS_ARG_REGA && arg->mem.base == 0)
		return true;

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
	{"mov", 1, {0x88}, JITAS_SIZE_BYTE, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x89}, JITAS_SIZE_ANY, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x8A}, JITAS_SIZE_BYTE, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{NULL, 1, {0x8B}, JITAS_SIZE_ANY, JITAS_ARG_MODRM, JITAS_ARG_REG},
	//... seg:offset movs 8C, 8E, A0, A1, A2, A3
	{NULL, 1, {0xB0}, JITAS_SIZE_BYTE, JITAS_ARG_IMM, JITAS_ARG_REG},
	{NULL, 1, {0xB8}, JITAS_SIZE_ANY, JITAS_ARG_IMM, JITAS_ARG_REG},
	{NULL, 1, {0xC6, 0}, JITAS_SIZE_BYTE, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{NULL, 1, {0xC7, 0}, JITAS_SIZE_ANY, JITAS_ARG_IMM_MAX32, JITAS_ARG_MODRM},

	{"lea", 1, {0x8D}, JITAS_SIZE_ANY, JITAS_ARG_MODRM, JITAS_ARG_REG},

	{"inc", 1, {0xFE, 0}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xFF, 0}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},

	{"add", 1, {0x04}, JITAS_SIZE_BYTE, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{NULL, 1, {0x05}, JITAS_SIZE_ANY, JITAS_ARG_IMM_MAX32, JITAS_ARG_REGA},
	{NULL, 1, {0x80, 0}, JITAS_SIZE_BYTE, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{NULL, 1, {0x81, 0}, JITAS_SIZE_ANY, JITAS_ARG_IMM_MAX32, JITAS_ARG_MODRM},
	{NULL, 1, {0x83, 0}, JITAS_SIZE_ANY, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0x00}, JITAS_SIZE_BYTE, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x01}, JITAS_SIZE_ANY, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x02}, JITAS_SIZE_BYTE, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{NULL, 1, {0x03}, JITAS_SIZE_ANY, JITAS_ARG_MODRM, JITAS_ARG_REG},

	{"dec", 1, {0xFE, 1}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xFF, 1}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},

	{"sub", 1, {0x2C}, JITAS_SIZE_BYTE, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{NULL, 1, {0x2D}, JITAS_SIZE_ANY, JITAS_ARG_IMM_MAX32, JITAS_ARG_REGA},
	{NULL, 1, {0x80, 5}, JITAS_SIZE_BYTE, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{NULL, 1, {0x81, 5}, JITAS_SIZE_ANY, JITAS_ARG_IMM_MAX32, JITAS_ARG_MODRM},
	{NULL, 1, {0x83, 5}, JITAS_SIZE_ANY, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0x28}, JITAS_SIZE_BYTE, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x29}, JITAS_SIZE_ANY, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x2A}, JITAS_SIZE_BYTE, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{NULL, 1, {0x2B}, JITAS_SIZE_ANY, JITAS_ARG_MODRM, JITAS_ARG_REG},

	{"mul", 1, {0xF6, 4}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xF7, 4}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},

	{"div", 1, {0xF6, 6}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xF7, 6}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},

	{"shl", 1, {0xD0, 4}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD1, 4}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD2, 4}, JITAS_SIZE_BYTE, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xD3, 4}, JITAS_SIZE_ANY, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xC0, 4}, JITAS_SIZE_BYTE, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0xC1, 4}, JITAS_SIZE_ANY, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"sal", 1, {0xD0, 4}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD1, 4}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD2, 4}, JITAS_SIZE_BYTE, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xD3, 4}, JITAS_SIZE_ANY, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xC0, 4}, JITAS_SIZE_BYTE, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0xC1, 4}, JITAS_SIZE_ANY, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"shr", 1, {0xD0, 5}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD1, 5}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD2, 5}, JITAS_SIZE_BYTE, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xD3, 5}, JITAS_SIZE_ANY, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xC0, 5}, JITAS_SIZE_BYTE, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0xC1, 5}, JITAS_SIZE_ANY, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"sar", 1, {0xD0, 7}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD1, 7}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0xD2, 7}, JITAS_SIZE_BYTE, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xD3, 7}, JITAS_SIZE_ANY, JITAS_ARG_REGCL, JITAS_ARG_MODRM},
	{NULL, 1, {0xC0, 7}, JITAS_SIZE_BYTE, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0xC1, 7}, JITAS_SIZE_ANY, JITAS_ARG_IMM8, JITAS_ARG_MODRM},

	{"cmp", 1, {0x3C}, JITAS_SIZE_BYTE, JITAS_ARG_IMM, JITAS_ARG_REGA},
	{NULL, 1, {0x3D}, JITAS_SIZE_ANY, JITAS_ARG_IMM_MAX32, JITAS_ARG_REGA},
	{NULL, 1, {0x80, 7}, JITAS_SIZE_BYTE, JITAS_ARG_IMM, JITAS_ARG_MODRM},
	{NULL, 1, {0x81, 7}, JITAS_SIZE_ANY, JITAS_ARG_IMM_MAX32, JITAS_ARG_MODRM},
	{NULL, 1, {0x83, 7}, JITAS_SIZE_BYTE, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0x83, 7}, JITAS_SIZE_ANY, JITAS_ARG_IMM8, JITAS_ARG_MODRM},
	{NULL, 1, {0x38}, JITAS_SIZE_BYTE, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{NULL, 1, {0x39}, JITAS_SIZE_ANY, JITAS_ARG_MODRM, JITAS_ARG_REG},
	{NULL, 1, {0x3A}, JITAS_SIZE_BYTE, JITAS_ARG_REG, JITAS_ARG_MODRM},
	{NULL, 1, {0x3B}, JITAS_SIZE_ANY, JITAS_ARG_REG, JITAS_ARG_MODRM},

	//{"jmp", 1, {0xEB}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jmp", 1, {0xE9}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_REL32},
	{NULL, 1, {0xFF, 4}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	//... far jmp

	{"jo", 1, {0x70}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jno", 1, {0x71}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jb", 1, {0x72}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jc", 1, {0x72}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnae", 1, {0x72}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnb", 1, {0x73}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnc", 1, {0x73}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jae", 1, {0x73}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"je", 1, {0x74}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jz", 1, {0x74}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jne", 1, {0x75}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnz", 1, {0x75}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jbe", 1, {0x76}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jna", 1, {0x76}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnbe", 1, {0x77}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"ja", 1, {0x77}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"js", 1, {0x78}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jns", 1, {0x79}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jp", 1, {0x7A}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jpe", 1, {0x7A}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jpo", 1, {0x7B}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnp", 1, {0x7B}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jl", 1, {0x7C}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnge", 1, {0x7C}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnl", 1, {0x7D}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jge", 1, {0x7D}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jle", 1, {0x7E}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jng", 1, {0x7E}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jnle", 1, {0x7F}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},
	{"jg", 1, {0x7F}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_REL8},

	{"call", 1, {0xE8}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_REL32},
	{NULL, 1, {0xFF, 2}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	//... far call

	{"ret", 1, {0xC3}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_NONE},
	{NULL, 1, {0xC2}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_IMM16},
	//... far ret

	{"push", 1, {0xFF, 6}, JITAS_SIZE_PTR, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0x50}, JITAS_SIZE_PTR, JITAS_ARG_NONE, JITAS_ARG_REG},
	{NULL, 1, {0x6A}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_IMM},
	{NULL, 1, {0x68}, JITAS_SIZE_ANY, JITAS_ARG_NONE, JITAS_ARG_IMM_MAX32},
	//... push segment register

	{"pop", 1, {0x8F, 0}, JITAS_SIZE_PTR, JITAS_ARG_NONE, JITAS_ARG_MODRM},
	{NULL, 1, {0x50}, JITAS_SIZE_PTR, JITAS_ARG_NONE, JITAS_ARG_REG},
	//... pop segment register

	{"int3", 1, {0xCC}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_NONE},
	{"int", 1, {0xCD}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_IMM},

	{"sysenter", 2, {0x0F, 0x34}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_NONE},
	{"syscall", 2, {0x0F, 0x05}, JITAS_SIZE_BYTE, JITAS_ARG_NONE, JITAS_ARG_NONE},
};
int jitas_instructionCount = sizeof(jitas_instructions) / sizeof(jitas_instruction_t);
