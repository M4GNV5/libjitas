#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "jitas.h"

struct jitas_register
{
	const char *label;
	int8_t size;
	uint8_t id;
	bool withRex;
};

static int registerCount;
static struct jitas_register registerInfos[];

bool jitas_findRegister(const char *label, int8_t *size, uint8_t *id, bool *needsRex)
{
	int i;
	char buff[8];

	for(i = 0; label[i] != 0; i++)
		buff[i] = tolower(label[i]);
	buff[i] = 0;

	for(i = 0; i < registerCount; i++)
	{
		if(strcmp(registerInfos[i].label, buff) == 0)
		{
			*size = registerInfos[i].size;
			*id = registerInfos[i].id;

			if(needsRex != NULL)
				*needsRex = registerInfos[i].withRex;

			return true;
		}
	}

	return false;
}

bool jitas_findRegisterArg(const char *label, jitas_argument_t *arg)
{
	arg->type = JITAS_ARG_REG;
	arg->mem.scale = 0;
	arg->mem.offset = 0;
	return jitas_findRegister(label, &arg->size, &arg->mem.base, &arg->needsRex);
}

static struct jitas_register registerInfos[] = {
	{"al", 1, 0, false},
	{"ah", 1, 4, false},
	{"ax", 2, 0, false},
	{"eax", 4, 0, false},
	{"rax", 8, 0, false},

	{"cl", 1, 1, false},
	{"ch", 1, 5, false},
	{"cx", 2, 1, false},
	{"ecx", 4, 1, false},
	{"rcx", 8, 1, false},

	{"dl", 1, 2, false},
	{"dh", 1, 6, false},
	{"dx", 2, 2, false},
	{"edx", 4, 2, false},
	{"rdx", 8, 2, false},

	{"bl", 1, 3, false},
	{"bh", 1, 7, false},
	{"bx", 2, 3, false},
	{"ebx", 4, 3, false},
	{"rbx", 8, 3, false},

	{"spl", 1, 4, true},
	{"sp", 2, 4, false},
	{"esp", 4, 4, false},
	{"rsp", 8, 4, false},

	{"bpl", 1, 5, true},
	{"bp", 2, 5, false},
	{"ebp", 4, 5, false},
	{"rbp", 8, 5, false},

	{"sil", 1, 6, true},
	{"si", 2, 6, false},
	{"esi", 4, 6, false},
	{"rsi", 8, 6, false},

	{"dil", 1, 7, true},
	{"di", 2, 7, false},
	{"edi", 4, 7, false},
	{"rdi", 8, 7, false},

	{"r8b", 1, 8, true},
	{"r8w", 2, 8, true},
	{"r8d", 4, 8, true},
	{"r8", 8, 8, true},

	{"r8b", 1, 9, true},
	{"r9w", 2, 9, true},
	{"r9d", 4, 9, true},
	{"r9", 8, 9, true},

	{"r10b", 1, 10, true},
	{"r10w", 2, 10, true},
	{"r10d", 4, 10, true},
	{"r10", 8, 10, true},

	{"r11b", 1, 11, true},
	{"r11w", 2, 11, true},
	{"r11d", 4, 11, true},
	{"r11", 8, 11, true},

	{"r12b", 1, 12, true},
	{"r12w", 2, 12, true},
	{"r12d", 4, 12, true},
	{"r12", 8, 12, true},

	{"r13b", 1, 13, true},
	{"r13w", 2, 13, true},
	{"r13d", 4, 13, true},
	{"r13", 8, 13, true},

	{"r14b", 1, 14, true},
	{"r14w", 2, 14, true},
	{"r14d", 4, 14, true},
	{"r14", 8, 14, true},
};
static int registerCount = sizeof(registerInfos) / sizeof(struct jitas_register);
