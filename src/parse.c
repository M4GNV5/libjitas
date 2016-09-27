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

struct errorlist
{
	char *msg;
	int line;
	struct errorlist *next;
};
static struct errorlist *firstError = NULL;
static struct errorlist *lastError = NULL;

static void addError(char *msg, int line)
{
	struct errorlist *curr = malloc(sizeof(struct errorlist));

	curr->msg = msg;
	curr->line = line;
	curr->next = NULL;
	if(lastError == NULL)
		firstError = curr;
	else
		lastError->next = curr;

	lastError = curr;
}

char *jitas_error(int *line)
{
	if(firstError == NULL)
		return NULL;

	struct errorlist *curr = firstError;
	firstError = firstError->next;
	if(firstError == NULL)
		lastError = NULL;

	char *msg = curr->msg;
	if(line != NULL)
		*line = curr->line;

	free(curr);
	return msg;
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
	int line = 0;

	ctx->symbols = NULL;
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

		line++;
		int len = parseIdentifier(&str, buff, 32);
		if(len == 0)
		{
			buff[31] = 0;
			errbuff = malloc(128);
			sprintf(errbuff, "Unexpected '%s'", buff);
		 	addError(errbuff, line);
			skipToNewline(&str);
			continue;
		}

		skipSpaces(&str);
		argStart = str;
		if(!parseArg(&str, &arg0))
		{
			errbuff = malloc(64 + str - argStart);
			sprintf(errbuff, "Invalid argument '%.*s'", (int)(str - argStart), argStart);
			addError(errbuff, line);
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
				errbuff = malloc(64 + str - argStart);
				sprintf(errbuff, "Invalid argument '%.*s'", (int)(str - argStart), argStart);
				addError(errbuff, line);
				skipToNewline(&str);
				continue;
			}

			src = &arg0;
			dst = &arg1;
		}
		else
		{
			arg1.type = JITAS_ARG_NONE;
			arg1.size = 0;
			src = &arg1;
			dst = &arg0;
		}

		if(arg0.size == 0 && arg1.size == 0)
		{
			switch(buff[len - 1])
			{
				case 'b':
					arg0.size = 1;
					arg1.size = 1;
					break;
				case 'w':
					arg0.size = 2;
					arg1.size = 2;
					break;
				case 'l':
					arg0.size = 4;
					arg1.size = 4;
					break;
				case 'q':
					arg0.size = 8;
					arg1.size = 8;
					break;
				default:
					errbuff = malloc(64);
					sprintf(errbuff, "Instruction requires size suffix");
					addError(errbuff, line);
					skipToNewline(&str);
					continue;
			}

			buff[len - 1] = 0;
		}
		else if(arg0.size == 0 && arg1.size != 0)
		{
			arg0.size = arg1.size;
		}
		else if(arg0.size != 0 && arg1.size == 0)
		{
			arg1.size = arg0.size;
		}


		skipSpaces(&str);
		if(*str != '\n' && *str != 0)
		{
			errbuff = malloc(128);
			sprintf(errbuff, "Expected line end at line %d", line);
		 	addError(errbuff, line);
			skipToNewline(&str);
			continue;
		}

		if(*str != 0)
			str++;

		ins = jitas_findInstruction(buff, src, dst);
		if(ins == NULL)
		{
			char last = buff[len - 1];
			switch(last)
			{
				case 'b':
					last = 1;
					break;
				case 'w':
					last = 2;
					break;
				case 'l':
					last = 4;
					break;
				case 'q':
					last = 8;
					break;
				default:
					last = 0;
			}

			if(last != 0)
			{
				if(last != src->size && last != dst->size)
				{
					errbuff = malloc(128);
					if(src->size == dst->size)
						sprintf(errbuff, "Instruction suffix is for size %d but arguments have size %d",
							last, src->size);
					else
						sprintf(errbuff, "Instruction suffix is for size %d but arguments have size %d/%d",
							last, src->size, dst->size);

					addError(errbuff, line);
					continue;
				}

				buff[len - 1] = 0;
				ins = jitas_findInstruction(buff, src, dst);
			}

			if(ins == NULL)
			{
				addError(jitas_errorMsg(buff, src, dst), line);
				skipToNewline(&str);
				continue;
			}
		}

		jitas_encode(ctx, ins, src, dst);
	}

	int len = ctx->ptr - startPtr;
	ctx->ptr = startPtr;
	return len;
}

bool jitas_link(jitas_context_t *ctx, void *data)
{
	jitas_symboltable_t *curr = ctx->symbols;
	while(curr != NULL)
	{
		uint8_t *resolved = ctx->resolver(curr->symbol, data);
		ptrdiff_t diff = resolved - curr->nextInsPtr;

		if(resolved == NULL)
		{
			char *err = malloc(64);
			sprintf(err, "Symbol resolver returned NULL for symbol '%s'", curr->symbol);
			addError(err, -1);
			return false;
		}
		else if((curr->size == 1 && (diff < INT8_MIN || diff > INT8_MAX))
			|| (curr->size == 4 && (diff < INT32_MIN || diff > INT32_MAX)))
		{
			char *err = malloc(256);
			sprintf(err, "Distance to symbol '%s' is too far for a rel%s jump/call",
				curr->symbol, curr->size == 1 ? "8" : "32");
			addError(err, -1);
			return false;
		}

		if(curr->size == 1)
		{
			*(curr->ptr) = diff;
		}
		else if(curr->size == 4)
		{
			*(int32_t *)(curr->ptr) = diff;
		}

		curr = curr->next;
	}
	return true;
}
