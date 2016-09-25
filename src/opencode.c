#include "jitas.h"

typedef struct
{
	uint8_t rm : 3;
	uint8_t reg : 3;
	uint8_t mod : 2;
} __attribute__((packed)) modrm_byte_t;

typedef struct
{
	uint8_t base : 3;
	uint8_t index : 3;
	uint8_t scale : 2;
} __attribute__((packed)) sib_byte_t;

static int jitas_placeArg(uint8_t *ptr, jitas_argument_t *arg)
{
	if(arg->type == JITAS_ARG_IMM8)
	{
		*ptr = arg->imm;
		return 1;
	}
	else if(arg->type == JITAS_ARG_IMM)
	{
		switch(arg->size)
		{
			case 1:
				*ptr = arg->imm;
				break;
			case 2:
				*(int16_t *)ptr = arg->imm;
				break;
			case 4:
				*(int32_t *)ptr = arg->imm;
				break;
			case 8:
				*(int64_t *)ptr = arg->imm;
				break;
		}
		return arg->size;
	}
	else
	{
		return 0;
	}
}

int jitas_encode(uint8_t *ptr, jitas_instruction_t *ins, jitas_argument_t *src, jitas_argument_t *dst)
{
	uint8_t *startPtr = ptr;

	if(src->needsRex || dst->needsRex)
	{
		*ptr = 0x40;
		if(src->size == 8 || dst->size == 8)
			*ptr |= 0b1000;
		ptr++;
	}

	for(int i = 0; i < ins->oplen; i++)
		*ptr++ = ins->opcode[i];

	if(ins->source == JITAS_ARG_MODRM || ins->destination == JITAS_ARG_MODRM)
	{
		modrm_byte_t modrm;

		//rearrange dst=MODRM src=REG/IMM/...
		jitas_argtype_t srcType = ins->source;
		if(ins->source == JITAS_ARG_MODRM)
		{
			srcType = ins->destination;
			jitas_argument_t *tmp = src;
			src = dst;
			dst = tmp;
		}

		if(srcType == JITAS_ARG_REG)
		{
			modrm.reg = src->mem.base & 7;
			if(src->needsRex && src->mem.base > 7)
				*startPtr |= 0b0100;
		}
		else
		{
			modrm.reg = ins->opcode[ins->oplen];
		}

		if(dst->type == JITAS_ARG_REG)
			modrm.mod = 0b11;
		else if(dst->mem.offset == 0)
			modrm.mod = 0b00;
		else if(dst->mem.offset >= INT8_MIN && dst->mem.offset <= INT8_MAX)
			modrm.mod = 0b01;
		else
			modrm.mod = 0b10;

		if(dst->needsRex && dst->mem.base > 7)
			*startPtr |= 0b0001;

		if(dst->mem.scale == 0)
		{
			modrm.rm = dst->mem.base & 7;
			*ptr++ = *(uint8_t *)&modrm;
		}
		else
		{
			sib_byte_t sib;
			modrm.rm = 0b100;

			sib.base = dst->mem.base & 7;
			sib.index = dst->mem.index & 7;
			if(dst->needsRex && dst->mem.index > 7)
				*startPtr |= 0b0010;

			switch(dst->mem.scale)
			{
				case 1:
					sib.scale = 0;
					break;
				case 2:
					sib.scale = 1;
					break;
				case 4:
					sib.scale = 2;
					break;
				case 8:
					sib.scale = 3;
					break;
			}

			*ptr++ = *(uint8_t *)&modrm;
			*ptr++ = *(uint8_t *)&sib;
		}

		switch(modrm.mod)
		{
			case 0b01:
				*(int8_t *)ptr++ = dst->mem.offset;
				break;
			case 0b10:
				*(int32_t *)ptr++ = dst->mem.offset;
				break;
		}

		if(srcType != JITAS_ARG_REG)
			ptr += jitas_placeArg(ptr, src);

		return ptr - startPtr;
	}
	else if(ins->source == JITAS_ARG_REG)
	{
		ptr--;
		*ptr &= 0xF8;
		*ptr |= src->mem.base & 7;
		ptr++;

		if(src->needsRex && src->mem.base > 7)
			*startPtr |= 0b0001;

		return ptr - startPtr + jitas_placeArg(ptr, dst);
	}
	else if(ins->destination == JITAS_ARG_REG)
	{
		ptr--;
		*ptr &= 0xF8;
		*ptr |= dst->mem.base & 0b111;
		ptr++;

		if(dst->needsRex && dst->mem.base > 7)
			*startPtr |= 1;

		return ptr - startPtr + jitas_placeArg(ptr, src);
	}
	else
	{
		int size = ptr - startPtr;
		size += jitas_placeArg(ptr, src);
		size += jitas_placeArg(ptr, dst);

		return size;
	}
}
