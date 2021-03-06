#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "jitas.h"

static const char *argTypeTexts[][4] = {
	[JITAS_ARG_REG] = {"r8", "r16", "r32", "r64"},
	[JITAS_ARG_IMM] = {"imm8", "imm16", "imm32", "imm64"},
	[JITAS_ARG_MODRM] = {"r/m8", "r/m16", "r/m32", "r/m64"},
	[JITAS_ARG_SYMBOL] = {"r/m8", "r/m16", "r/m32", "r/m64"},
	[JITAS_ARG_SYMBOL_ADDRESS] = {"imm8", "imm16", "imm32", "imm64"},
};

static const char *insTypeTexts[][6] = {
	//common
	[JITAS_ARG_NONE] = {NULL, NULL, NULL, NULL, NULL, NULL},
	[JITAS_ARG_REG] = {"???", "r8", "r64", "r16", "r32", "r64"},
	[JITAS_ARG_IMM] = {"???", "imm8", "imm64", "imm16", "imm32", "imm64"},
	[JITAS_ARG_MODRM] = {"r/m", "r/m8", "r/m64", "r/m16", "r/m32", "r/m64"},

	//special
	[JITAS_ARG_IMM8] = {"imm8", "imm8", "imm8", "imm8", "imm8", "imm8"},
	[JITAS_ARG_IMM16] = {"imm16", "imm16", "imm16", "imm16", "imm16", "imm16"},
	[JITAS_ARG_IMM_MAX32] = {"???", "imm8", "imm64", "imm16", "imm32", "imm32"},
	[JITAS_ARG_REL8] = {"rel8", "rel8", "rel8", "rel8", "rel8", "rel8"},
	[JITAS_ARG_REL32] = {"rel32", "rel32", "rel32", "rel32", "rel32", "rel32"},
	[JITAS_ARG_REGA] = {"???", "%al", "%rax", "%ax", "%eax", "%rax"},
	[JITAS_ARG_REGCL] = {"%cl", "%cl", "%cl", "%cl", "%cl", "%cl"},
};

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
		default:
			assert(0);
	}
}

char *jitas_findInstructionError(const char *label, jitas_argument_t *src, jitas_argument_t *dst)
{
	for(int i = 0; i < jitas_instructionCount;)
	{
		if(strcmp(jitas_instructions[i].label, label) == 0)
		{
			int buffLen = 1024;
			char *startBuff = malloc(1024);
			char *buff = startBuff;

			buff += sprintf(buff, "Invalid instruction use: '%s", label);

			if(src->type != JITAS_ARG_NONE)
				buff += sprintf(buff, " %s, %s'", argTypeTexts[src->type][convertSize(src->size)],
					argTypeTexts[dst->type][convertSize(dst->size)]);
			else if(dst->type != JITAS_ARG_NONE)
				buff += sprintf(buff, " %s'", argTypeTexts[dst->type][convertSize(dst->size)]);
			else
				buff += sprintf(buff, "'");

			buff += sprintf(buff, " Valid overloads for '%s' are:\n", label);

			do
			{
				jitas_instruction_t *ins = jitas_instructions + i;

				if(buff - startBuff >= buffLen - 128)
				{
					buffLen *= 2;
					char *newBuff = realloc(startBuff, buffLen);
					buff = newBuff + (buff - startBuff);
					startBuff = newBuff;
				}

				const char *fmt;
				int arg0;
				int arg1;

				if(ins->source != JITAS_ARG_NONE)
				{
					fmt = "    %s %s, %s\n";
					arg0 = ins->source;
					arg1 = ins->destination;
				}
				else if(ins->destination != JITAS_ARG_NONE)
				{
					fmt = "    %s %s\n";
					arg0 = ins->destination;
					arg1 = 0;
				}
				else
				{
					fmt = "    %s\n";
					arg0 = 0;
					arg1 = 0;
				}

				buff += sprintf(buff, fmt, label,
					insTypeTexts[arg0][ins->size],
					insTypeTexts[arg1][ins->size]);
				if(ins->size == JITAS_SIZE_ANY && ins->destination != JITAS_ARG_NONE)
				{
					buff += sprintf(buff, fmt, label,
						insTypeTexts[arg0][JITAS_SIZE_ANY + 1],
						insTypeTexts[arg1][JITAS_SIZE_ANY + 1]);

					buff += sprintf(buff, fmt, label,
						insTypeTexts[arg0][JITAS_SIZE_ANY + 2],
						insTypeTexts[arg1][JITAS_SIZE_ANY + 2]);
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

	int len = snprintf(NULL, 0, "Unknown instruction '%s'", label) + 1;
	char *buff = malloc(len);
	sprintf(buff, "Unknown instruction '%s'", label);
	return buff;
}

struct errorlist
{
	char *msg;
	int line;
	struct errorlist *next;
};

void jitas_addError(jitas_context_t *ctx, char *msg, int line)
{
	struct errorlist *curr = malloc(sizeof(struct errorlist));

	curr->msg = msg;
	curr->line = line;
	curr->next = NULL;
	if(ctx->lastError == NULL)
		ctx->firstError = curr;
	else
		ctx->lastError->next = curr;

	ctx->lastError = curr;
}

char *jitas_error(jitas_context_t *ctx, int *line)
{
	if(ctx->firstError == NULL)
		return NULL;

	struct errorlist *curr = ctx->firstError;
	ctx->firstError = curr->next;
	if(ctx->firstError == NULL)
		ctx->lastError = NULL;

	char *msg = curr->msg;
	if(line != NULL)
		*line = curr->line;

	free(curr);
	return msg;
}
