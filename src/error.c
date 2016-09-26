#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jitas.h"

static const char *argtypes[][4] = {
	//common
	[JITAS_ARG_NONE] = {NULL, NULL, NULL, NULL},
	[JITAS_ARG_REG] = {"r8", "r16", "r32", "r64"},
	[JITAS_ARG_IMM] = {"imm8", "imm16", "imm32", "imm64"},
	[JITAS_ARG_MODRM] = {"r/m8", "r/m16", "r/m32", "r/m64"},

	//special
	[JITAS_ARG_IMM8] = {"imm8", "imm8", "imm8", "imm8"},
	[JITAS_ARG_IMM16] = {"imm16", "imm16", "imm16", "imm16"},
	[JITAS_ARG_IMM_MAX32] = {"imm8", "imm16", "imm32", "imm32"},
	[JITAS_ARG_REGA] = {"%al", "%ax", "%eax", "%rax"},
	[JITAS_ARG_REGCL] = {"%cl", "%cl", "%cl", "%cl"},
};
static const char sizeExtensions[] = {'b', 'w', 'l', 'q'};

static int convertSize(int size)
{
	switch(size)
	{
		case 1:
			return 0;
		case 2:
			return 1;
		case 4:
			return 2;
		case 8:
			return 3;
	}

	//TODO raise SIGABRT?
	return 0;
}

char *jitas_errorMsg(const char *label, int size, jitas_argument_t *src, jitas_argument_t *dst)
{
	size = convertSize(size);

	for(int i = 0; i < jitas_instructionCount;)
	{
		if(strcmp(jitas_instructions[i].label, label) == 0)
		{
			int buffLen = 1024;
			char *startBuff = malloc(1024);
			char *buff = startBuff;

			buff += sprintf(buff, "Invalid instruction use: '%s", label);

			if(src->type != JITAS_ARG_NONE)
				buff += sprintf(buff, " %s, %s'\n", argtypes[src->type][convertSize(src->size)],
					argtypes[dst->type][convertSize(dst->size)]);
			else if(dst->type != JITAS_ARG_NONE)
				buff += sprintf(buff, " %s'\n", argtypes[dst->type][convertSize(dst->size)]);
			else
				buff += sprintf(buff, "'\n");

			buff += sprintf(buff, "Valid overloads for '%s%c' are:\n", label, sizeExtensions[size]);

			do
			{
				jitas_instruction_t *ins = jitas_instructions + i;

				if((ins->isByteOp && size == 0) || (!ins->isByteOp && size != 0))
				{
					if(buff - startBuff >= buffLen - 128)
					{
						buffLen *= 2;
						char *newBuff = realloc(startBuff, buffLen);
						buff = newBuff + (buff - startBuff);
						startBuff = newBuff;
					}

					if(ins->source != JITAS_ARG_NONE)
						buff += sprintf(buff, "    %s %s, %s\n", label, argtypes[ins->source][size], argtypes[ins->destination][size]);
					else if(ins->destination != JITAS_ARG_NONE)
						buff += sprintf(buff, "    %s %s\n", label, argtypes[ins->destination][size]);
					else
						buff += sprintf(buff, "    %s\n", label);
				}
				i++;
			} while(jitas_instructions[i].label == NULL);

			return startBuff;
		}
		else
		{
			do
			{
				i++;
			} while(jitas_instructions[i].label == NULL);
		}
	}

	int len = snprintf(NULL, 0, "Unknown instruction '%s'\n", label) + 1;
	char *buff = malloc(len);
	sprintf(buff, "Unknown instruction '%s'\n", label);
	return buff;
}
