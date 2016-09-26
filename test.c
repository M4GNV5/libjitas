#include <stdio.h>

#include "src/jitas.h"

int main()
{
	const char *insLabel = "mov";
	int insSize = 2;
	jitas_argument_t src = {
		.type = JITAS_ARG_MODRM,
		.size = 2,
		.mem = {
			.base = 4,
			.index = 0,
			.scale = 0,
			.offset = 42
		}
	};
	jitas_argument_t dst;
	if(!jitas_findRegisterArg("r10w", &dst))
	{
		fprintf(stderr, "Invalid register\n");
		return 1;
	}

	jitas_instruction_t *ins = jitas_findInstruction(insLabel, &src, &dst);

	if(ins == NULL)
	{
		fprintf(stderr, "%s", jitas_errorMsg(insLabel, insSize, &src, &dst));
		return 1;
	}

	uint8_t buff[32] = {0};
	int len = jitas_encode(buff, ins, &src, &dst);

	for(int i = 0; i < len; i++)
	{
		printf("%02hhX ", buff[i]);
	}
	printf("\n");
	return 0;
}
