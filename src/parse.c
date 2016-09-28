#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>

#include "jitas.h"

static void skipSpaces(const char **str)
{
	while(isblank(*(*str)++));
	(*str)--;
}

static void skipToNewline(const char **str)
{
	char c;
	do
	{
		c = *(*str)++;
	} while(c != '\n' && c != 0);
}

static int parseIdentifier(const char **str, char *buff, int maxlen)
{
	int len = 0;
	char c = *(*str)++;

	while(isalnum(c) && len <= maxlen)
	{
		*buff++ = c;
		c = *(*str)++;
		len++;
	}

	(*str)--;
	if(len == maxlen)
		return 0;

	*buff = 0;
	return len;
}

static bool parseArg(const char **str, jitas_argument_t *arg)
{
	char buff[128];
	char curr = **str;
	switch(curr)
	{
		case '\n':
		case 0:
			arg->type = JITAS_ARG_NONE;
			break;
		case '%':
			(*str)++;
			if(parseIdentifier(str, buff, 128) == 0)
				return false;
			jitas_findRegisterArg(buff, arg);
			break;
		case '$':
			(*str)++;
			if(isdigit(**str) || **str == '-')
			{
				arg->type = JITAS_ARG_IMM;
				arg->size = 0;
				arg->needsRex = false;
				arg->imm = strtol(*str, (void *)str, 0);
			}
			/*else if(isalpha(**str))
			{
				//TODO label
			}*/
			else
			{
				return false;
			}
			break;
		default:
			if(isdigit(curr) || curr == '-' || curr == '(')
			{
				arg->type = JITAS_ARG_MODRM;
				arg->size = 0;
				arg->needsRex = false;

				if(curr != '(')
					arg->mem.offset = strtol(*str, (void *)str, 0);
				else
					arg->mem.offset = 0;

				if(*(*str)++ != '(')
					return false;

				skipSpaces(str);
				if(*(*str)++ != '%')
					return false;

				int8_t size;

				if(parseIdentifier(str, buff, 128) == 0)
					return false;
				jitas_findRegister(buff, &size, &arg->mem.base, &arg->needsRex);
				if(size != sizeof(void *))
					return false;

				skipSpaces(str);
				curr = *(*str)++;
				if(curr == ',')
				{
					skipSpaces(str);
					if(*(*str)++ != '%')
						return false;

					bool needsRex;

					if(parseIdentifier(str, buff, 128) == 0)
						return false;
					jitas_findRegister(buff, &size, &arg->mem.index, &needsRex);
					arg->needsRex = arg->needsRex || needsRex;
					if(size != sizeof(void *))
						return false;

					skipSpaces(str);
					if(*(*str)++ != ',')
						return false;

					skipSpaces(str);
					curr = *(*str)++;
					if(curr != '1' && curr != '2' && curr != '4' && curr != '8')
						return false;
					arg->mem.scale = curr - '0';

					skipSpaces(str);
					curr = *(*str)++;
				}
				else
				{
					arg->mem.scale = 0;
					arg->mem.index = 0;
				}

				if(curr != ')')
					return false;
			}
			else if(isalpha(**str))
			{
				if(parseIdentifier(str, buff, 128) == 0)
					return false;

				arg->type = JITAS_ARG_SYMBOL;
				arg->size = 8;
				arg->needsRex = false;
				arg->symbol = strdup(buff);
			}
			else
			{
				return false;
			}
	}
	return true;
}

static int sizeFromSuffix(char suffix)
{
	switch(suffix)
	{
		case 'b':
			return 1;
		case 'w':
			return 2;
		case 'l':
			return 4;
		case 'q':
			return 8;
	}
	return -1;
}

static jitas_instruction_t *adjustSizeAndFindInstruction(jitas_context_t *ctx, int len, char *buff,
	jitas_argument_t *src, jitas_argument_t *dst)
{
	jitas_instruction_t *ins;
	char *errbuff;

	ins = jitas_findInstruction(buff, src, dst);
	if(ins != NULL)
	{
		//size mismatches are probably okay here
	}
	else if(src->size == 0 && dst->size == 0)
	{
		int size = sizeFromSuffix(buff[len - 1]);
		if(size < 0)
		{
			asprintf(&errbuff, "Instruction requires size suffix");
			jitas_addError(ctx, errbuff, ctx->line);
			return NULL;
		}

		src->size = size;
		dst->size = size;
		buff[len - 1] = 0;
	}
	else if(src->size == 0 && dst->size != 0)
	{
		src->size = dst->size;
	}
	else if(src->size != 0 && dst->size == 0)
	{
		dst->size = src->size;
	}

	ins = jitas_findInstruction(buff, src, dst);
	if(ins == NULL)
	{
		int size = sizeFromSuffix(buff[len - 1]);
		if(size > 0)
		{
			if(size != src->size && size != dst->size)
			{
				if(src->size == dst->size)
					asprintf(&errbuff, "Instruction suffix is for size %d but arguments have size %d",
						size, src->size);
				else
					asprintf(&errbuff, "Instruction suffix is for size %d but arguments have size %d/%d",
						size, src->size, dst->size);

				jitas_addError(ctx, errbuff, ctx->line);
				return NULL;
			}

			buff[len - 1] = 0;
			ins = jitas_findInstruction(buff, src, dst);
		}

		if(ins == NULL)
			jitas_addError(ctx, jitas_findInstructionError(buff, src, dst), ctx->line);
	}

	return ins;
}

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
	char *errbuff;
	char buff[32];
	const char *argStart;
	jitas_argument_t arg0;
	jitas_argument_t arg1;
	jitas_argument_t *src;
	jitas_argument_t *dst;
	jitas_instruction_t *ins;

	ctx->line = 0;
	ctx->symbols = NULL;
	ctx->firstError = NULL;
	ctx->lastError = NULL;
	uint8_t *startPtr = ctx->ptr;

	while(*str != 0)
	{
		skipSpaces(&str);
		if(*str == '\n' || *str == 0)
		{
			if(*str == '\n')
				str++;
			continue;
		}

		ctx->line++;
		int len = parseIdentifier(&str, buff, 32);
		if(len == 0)
		{
			buff[31] = 0;
			asprintf(&errbuff, "Unexpected '%s'", buff);
		 	jitas_addError(ctx, errbuff, ctx->line);
			skipToNewline(&str);
			continue;
		}

		skipSpaces(&str);
		argStart = str;
		if(!parseArg(&str, &arg0))
		{
			asprintf(&errbuff, "Invalid argument '%.*s'", (int)(str - argStart), argStart);
			jitas_addError(ctx, errbuff, ctx->line);
			skipToNewline(&str);
			continue;
		}
		skipSpaces(&str);

		if(*str == ',')
		{
			str++;
			skipSpaces(&str);
			argStart = str;
			if(!parseArg(&str, &arg1))
			{
				asprintf(&errbuff, "Invalid argument '%.*s'", (int)(str - argStart), argStart);
				jitas_addError(ctx, errbuff, ctx->line);
				skipToNewline(&str);
				continue;
			}

			src = &arg0;
			dst = &arg1;
		}
		else
		{
			arg1.type = JITAS_ARG_NONE;
			arg1.needsRex = false;
			arg1.size = 0;
			src = &arg1;
			dst = &arg0;
		}

		skipSpaces(&str);
		if(*str != '\n' && *str != 0)
		{
			asprintf(&errbuff, "Expected line end at line %d", ctx->line);
		 	jitas_addError(ctx, errbuff, ctx->line);
			skipToNewline(&str);
			continue;
		}

		if(*str != 0)
			str++;

		//XXX hacky 8 byte address workaround for all symbol uses except conditional jumps
		//this places the absolute address into %r11 and replaces the symbol argument with (%r11)
		if((src->type == JITAS_ARG_SYMBOL || dst->type == JITAS_ARG_SYMBOL)
			&& (buff[0] != 'j' || strcmp(buff, "jmp") == 0))
		{
			if(src->type == JITAS_ARG_SYMBOL)
				hacky8ByteWorkaround(ctx, buff, src);
			else if(dst->type == JITAS_ARG_SYMBOL)
				hacky8ByteWorkaround(ctx, buff, dst);
		}

		ins = adjustSizeAndFindInstruction(ctx, len, buff, src, dst);
		if(ins == NULL)
			continue;

		jitas_encode(ctx, ins, src, dst);
	}

	int len = ctx->ptr - startPtr;
	ctx->ptr = startPtr;
	return len;
}

bool jitas_link(jitas_context_t *ctx, void *data)
{
	char *err;
	jitas_symboltable_t *curr = ctx->symbols;
	while(curr != NULL)
	{
		uint8_t *resolved = ctx->resolver(curr->symbol, data);
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
				*curr->ptr = diff;
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
