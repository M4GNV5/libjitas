#include <stdio.h>

#include "src/jitas.h"

int main()
{
	jitas_argument_t src = {
		.type = JITAS_ARG_MODRM,
		.size = 4,
		.mem = {
			.base = 0,
			.offset = 42,
			.scale = 0
		}
	};
	jitas_argument_t dst;
	jitas_findRegister("edx", &dst.size, &dst.mem.base, NULL);

	jitas_instruction_t *ins = jitas_findInstruction("mov", &src, &dst);

	uint8_t buff[32] = {0};
	int len = jitas_encode(buff, ins, &src, &dst);

	for(int i = 0; i < 32; i++)
	{
		printf("%02hhX ", buff[i]);
	}
	printf("\n");
}
