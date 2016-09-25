#include <stdio.h>

#include "jitas.h"

int main()
{
	jitas_argument_t src = {
		.type = JITAS_ARG_MODRM,
		.size = 4,
		.mem = {
			.base = 0,
			.offset = 0xFFFFFF
		}
	};
	jitas_argument_t dst = {
		.type = JITAS_ARG_REG,
		.size = 4,
		.mem = { .base = 2 },
	};

	jitas_instruction_t *ins = jitas_findInstruction("mov", &src, &dst);

	uint8_t buff[32] = {0};
	int len = jitas_encode(buff, ins, &src, &dst);

	for(int i = 0; i < 32; i++)
	{
		printf("%02hhX ", buff[i]);
	}
	printf("\n");
}
