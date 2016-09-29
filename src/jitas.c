#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "jitas.h"

static void hacky8ByteWorkaround(jitas_context_t *ctx, char *ins, jitas_argument_t *arg)
{
	if(sizeof(void *) != 8)
		return;

	jitas_symboltable_t *entry = malloc(sizeof(jitas_symboltable_t));
	entry->size = 8;
	entry->symbol = arg->symbol;
	entry->next = ctx->symbols;
	ctx->symbols = entry;

	//XXX workaround for 64bit addresses
	*ctx->ptr++ = 0x49; //rex.WB
	*ctx->ptr++ = 0xBB; //mov -> %r11
	entry->ptr = ctx->ptr;

	*(uint64_t *)ctx->ptr = 0;
	ctx->ptr += 8;
	entry->nextInsPtr = ctx->ptr;

	if(strcmp(ins, "jmp") == 0 || strcmp(ins, "call") == 0)
	{
		arg->type = JITAS_ARG_REG;
		arg->size = 8;
	}
	else
	{
		arg->type = JITAS_ARG_MODRM;
	}

	arg->needsRex = true;
	arg->mem.base = 11;
	arg->mem.offset = 0;
	arg->mem.scale = 0;
}

int jitas_assemble(jitas_context_t *ctx, const char *str)
{
	char buff[32];
	jitas_argument_t src;
	jitas_argument_t dst;
	jitas_instruction_t *ins;

	ctx->line = 0;
	ctx->symbols = NULL;
	ctx->localSymbols = NULL;
	ctx->firstError = NULL;
	ctx->lastError = NULL;
	uint8_t *startPtr = ctx->ptr;

	while(*str != 0)
	{
		if(!jitas_parse(&str, ctx, buff, &src, &dst))
			continue;

		//XXX hacky 8 byte address workaround for all symbol uses except conditional jumps
		//this places the absolute address into %r11 and replaces the symbol argument with (%r11)
		if((src.type == JITAS_ARG_SYMBOL || dst.type == JITAS_ARG_SYMBOL)
			&& (buff[0] != 'j' || strcmp(buff, "jmp") == 0))
		{
			if(src.type == JITAS_ARG_SYMBOL)
				hacky8ByteWorkaround(ctx, buff, &src);
			else if(dst.type == JITAS_ARG_SYMBOL)
				hacky8ByteWorkaround(ctx, buff, &dst);
		}

		ins = jitas_findUnsizedInstruction(ctx, buff, &src, &dst);
		if(ins == NULL)
			continue;

		jitas_encode(ctx, ins, &src, &dst);
	}

	int len = ctx->ptr - startPtr;
	ctx->ptr = startPtr;
	return len;
}

uint8_t *jitas_findLocalSymbol(jitas_context_t *ctx, const char *label)
{
	jitas_symboltable_t *curr = ctx->localSymbols;
	while(curr != NULL)
	{
		if(strcmp(curr->symbol, label) == 0)
			return curr->ptr;

		curr = curr->next;
	}
	return NULL;
}

bool jitas_link(jitas_context_t *ctx, void *data)
{
	char *err;
	jitas_symboltable_t *curr = ctx->symbols;
	while(curr != NULL)
	{
		uint8_t *resolved = jitas_findLocalSymbol(ctx, curr->symbol);
		if(resolved == NULL)
			resolved = ctx->resolver(curr->symbol, data);

		ptrdiff_t diff = resolved - curr->nextInsPtr;

		if(resolved == NULL)
		{
			asprintf(&err, "Symbol resolver returned NULL for symbol '%s'", curr->symbol);
			jitas_addError(ctx, err, -1);
			return false;
		}
		else if((curr->size == 1 && (diff < INT8_MIN || diff > INT8_MAX))
			|| (curr->size == 4 && (diff < INT32_MIN || diff > INT32_MAX)))
		{
			asprintf(&err, "Distance to symbol '%s' is too far for a rel%s jump/call",
				curr->symbol, curr->size == 1 ? "8" : "32");
			jitas_addError(ctx, err, -1);
			return false;
		}

		switch(curr->size)
		{
			case 1:
				*(int8_t *)curr->ptr = diff;
				break;
			case 2:
				*(int16_t *)curr->ptr = diff; //is this used/needed anywhere?
				break;
			case 4:
				*(int32_t *)curr->ptr = diff;
				break;
			case 8:
				*(void **)curr->ptr = resolved;
				break;
			default:
				asprintf(&err, "Cannot link symbol of size %d\n", curr->size);
				jitas_addError(ctx, err, -1);
		}

		curr = curr->next;
	}
	return true;
}
