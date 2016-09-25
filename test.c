#include <stdio.h>

#include "src/jitas.h"

int main()
{
	jitas_argument_t src = {
		.type = JITAS_ARG_IMM,
		.size = 4,
		.imm = 0xdeadc0de
	};
	jitas_argument_t dst;
	if(!jitas_findRegisterArg("ecx", &dst))
	{
		fprintf(stderr, "Invalid register\n");
		return 1;
	}

	if(src.type <= JITAS_ARG_MODRM && dst.type <= JITAS_ARG_MODRM && src.size != dst.size)
	{
		fprintf(stderr, "Size mismatch\n");
		return 1;
	}

	jitas_instruction_t *ins = jitas_findInstruction("add", &src, &dst);

	if(ins == NULL)
	{
		fprintf(stderr, "Invalid instruction\n");
		return 1;
	}

	uint8_t buff[32] = {0};
	int len = jitas_encode(buff, ins, &src, &dst);

	for(int i = 0; i < 32; i++)
	{
		printf("%02hhX ", buff[i]);
	}
	printf("\n");
	return 0;
}
