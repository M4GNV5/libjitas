#include <stdio.h>
#include <stdlib.h>

#include "src/jitas.h"

int main()
{
	/*const char *insLabel = "mov";
	jitas_argument_t src = {
		.type = JITAS_ARG_MODRM,
		.size = 8,
		.mem = {
			.base = 5,
			.index = 0,
			.scale = 0,
			.offset = 1337
		}
	};
	jitas_argument_t dst;
	if(!jitas_findRegisterArg("r10", &dst))
	{
		fprintf(stderr, "Invalid register\n");
		return 1;
	}

	jitas_instruction_t *ins = jitas_findInstruction(insLabel, &src, &dst);

	if(ins == NULL)
	{
		fprintf(stderr, "%s", jitas_errorMsg(insLabel, &src, &dst));
		return 1;
	}

	uint8_t buff[32] = {0};
	int len = jitas_encode(buff, ins, &src, &dst);*/

	//"mov $42, %rax\npush %rcx\nshl $5, 8(%rdi)\nadd %rdx 32(%rax,%rsi,4)"

	uint8_t buff[32] = {0};
	int len = jitas_assemble(buff, "mov $42, %rax\npush %rcx\nshl $5, 8(%rdi)\nadd %rdx, 32(%rax,%rsi,4)");

	for(;;)
	{
		char *err = jitas_error();
		if(err == NULL)
			break;

		fprintf(stderr, "%s\n", err);
		free(err);
	}

	if(len == 0)
		return 1;

	for(int i = 0; i < len; i++)
	{
		printf("%02hhX ", buff[i]);
	}
	printf("\n");
	return 0;
}
