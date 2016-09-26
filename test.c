#include <stdio.h>

#include "src/jitas.h"

int main()
{
	const char *insLabel = "shl";
	int insSize = 8;
	jitas_argument_t src;
	jitas_argument_t dst;
	if(!jitas_findRegisterArg("rdx", &dst) || !jitas_findRegisterArg("dl", &src))
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

	for(int i = 0; i < 32; i++)
	{
		printf("%02hhX ", buff[i]);
	}
	printf("\n");
	return 0;
}
