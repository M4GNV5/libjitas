#include <stdio.h>
#include <stdlib.h>
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

jitas_instruction_t *jitas_findUnsizedInstruction(jitas_context_t *ctx, char *buff,
	jitas_argument_t *src, jitas_argument_t *dst)
{
	int len = strlen(buff);
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

bool jitas_parse(const char **str, jitas_context_t *ctx, char *buff, jitas_argument_t *src, jitas_argument_t *dst)
{
	const char *argStart;
	char *errbuff;

	skipSpaces(str);
	if(**str == '\n' || **str == 0)
	{
		if(**str == '\n')
			(*str)++;
		return false;
	}

	int len = parseIdentifier(str, buff, 32);
	if(len == 0)
	{
		buff[31] = 0;
		asprintf(&errbuff, "Unexpected '%s'", buff);
		jitas_addError(ctx, errbuff, ctx->line);
		skipToNewline(str);
		return false;
	}

	if(**str == ':')
	{
		(*str)++;

		jitas_symboltable_t *entry = malloc(sizeof(jitas_symboltable_t));
		entry->size = 0;
		entry->symbol = strdup(buff);
		entry->ptr = entry->nextInsPtr = ctx->ptr;
		entry->next = ctx->localSymbols;
		ctx->localSymbols = entry;

		return false;
	}

	skipSpaces(str);
	argStart = *str;
	if(!parseArg(str, src))
	{
		asprintf(&errbuff, "Invalid argument '%.*s'", (int)(*str - argStart), argStart);
		jitas_addError(ctx, errbuff, ctx->line);
		skipToNewline(str);
		return false;
	}
	skipSpaces(str);

	if(**str == ',')
	{
		(*str)++;
		skipSpaces(str);
		argStart = *str;
		if(!parseArg(str, dst))
		{
			asprintf(&errbuff, "Invalid argument '%.*s'", (int)(*str - argStart), argStart);
			jitas_addError(ctx, errbuff, ctx->line);
			skipToNewline(str);
			return false;
		}
	}
	else
	{
		memcpy(dst, src, sizeof(jitas_argument_t));
		src->type = JITAS_ARG_NONE;
		src->needsRex = false;
		src->size = 0;
	}

	skipSpaces(str);
	if(**str != '\n' && **str != 0)
	{
		asprintf(&errbuff, "Expected line end at line %d", ctx->line);
		jitas_addError(ctx, errbuff, ctx->line);
		skipToNewline(str);
		return false;
	}

	if(**str != 0)
		(*str)++;

	return true;
}
