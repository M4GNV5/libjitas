#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

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
			/*else if(isalpha(**str))
			{
				//TODO label
			}*/
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
	struct errorlist *next;
};
static struct errorlist *firstError = NULL;
static struct errorlist *lastError = NULL;

static void addError(char *msg)
{
	struct errorlist *curr = malloc(sizeof(struct errorlist));

	curr->msg = msg;
	curr->next = NULL;
	if(lastError == NULL)
		firstError = curr;
	else
		lastError->next = curr;

	lastError = curr;
}

char *jitas_error()
{
	if(firstError == NULL)
		return NULL;

	struct errorlist *curr = firstError;
	firstError = firstError->next;
	if(firstError == NULL)
		lastError = NULL;

	char *msg = curr->msg;
	free(curr);
	return msg;
}

int jitas_assemble(uint8_t *ptr, const char *str)
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
	int byteCount = 0;

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
			sprintf(errbuff, "Unexpected '%s' at line %d\n", buff, line);
		 	addError(errbuff);
			skipToNewline(&str);
			continue;
		}

		skipSpaces(&str);
		argStart = str;
		if(!parseArg(&str, &arg0))
		{
			errbuff = malloc(64 + str - argStart);
			sprintf(errbuff, "Invalid argument '%.*s'\n", (int)(str - argStart), argStart);
			addError(errbuff);
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
				sprintf(errbuff, "Invalid argument '%.*s'\n", (int)(str - argStart), argStart);
				addError(errbuff);
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
					sprintf(errbuff, "Instruction requires size suffix\n");
					addError(errbuff);
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
			sprintf(errbuff, "Expected line end at line %d\n", line);
		 	addError(errbuff);
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
					sprintf(errbuff, "Instruction size suffix is for size %d but arguments have size %d and %d",
						last, src->size, dst->size);
					addError(errbuff);
					skipToNewline(&str);
					continue;
				}

				buff[len - 1] = 0;
				ins = jitas_findInstruction(buff, src, dst);
			}

			if(ins == NULL)
			{
				addError(jitas_errorMsg(buff, src, dst));
				skipToNewline(&str);
				continue;
			}
		}

		len = jitas_encode(ptr, ins, src, dst);
		byteCount += len;
		ptr += len;
	}
	return byteCount;
}
