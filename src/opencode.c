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

static int jitas_placeArg(uint8_t *ptr, jitas_argtype_t opArg, jitas_argument_t *arg)
{
	if(opArg == JITAS_ARG_IMM8)
	{
		*ptr = arg->imm;
		return 1;
	}
	else if(opArg == JITAS_ARG_IMM16)
	{
		*(int16_t *)ptr = arg->imm;
		return 2;
	}
	else if(opArg == JITAS_ARG_IMM || opArg == JITAS_ARG_IMM_MAX32)
	{
		int size = arg->size > 4 && opArg == JITAS_ARG_IMM_MAX32 ? 4 : arg->size;
		switch(size)
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
		return size;
	}
	else
	{
		return 0;
	}
}

int jitas_encode(uint8_t *ptr, jitas_instruction_t *ins, jitas_argument_t *src, jitas_argument_t *dst)
{
	uint8_t *startPtr = ptr;

	if(src->size == 2 || dst->size == 2)
		*ptr++ = 0x66;

	uint8_t *rexPtr = ptr;
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
				*rexPtr |= 0b0100;
		}
		else
		{
			modrm.reg = ins->opcode[ins->oplen];
		}

		if(dst->type == JITAS_ARG_REG)
			modrm.mod = 0b11;
		else if(dst->mem.offset == 0 && dst->mem.scale == 0 && (dst->mem.base & 7) == 5)
			modrm.mod = 0b01; //rbp+0 because modrm {mod 0, rm 5} is disp32(%rip)
		else if(dst->mem.offset == 0)
			modrm.mod = 0b00;
		else if(dst->mem.offset >= INT8_MIN && dst->mem.offset <= INT8_MAX)
			modrm.mod = 0b01;
		else
			modrm.mod = 0b10;

		if(dst->needsRex && dst->mem.base > 7)
			*rexPtr |= 0b0001;

		if(dst->mem.scale == 0 && (dst->mem.base & 7) != 4)
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
				*rexPtr |= 0b0010;

			switch(dst->mem.scale)
			{
				case 0:
					sib.index = 4;
					//fallthrough
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
				*(int32_t *)ptr = dst->mem.offset;
				ptr += 4;
				break;
		}

		if(modrm.mod == 0 && modrm.rm == 5)
			*(int32_t *)ptr++ = dst->mem.offset; //modrm {mod 0, rm 5} is disp32(%rip)

		if(srcType != JITAS_ARG_REG)
			ptr += jitas_placeArg(ptr, srcType, src);

		return ptr - startPtr;
	}
	else if(ins->source == JITAS_ARG_REG)
	{
		ptr--;
		*ptr &= 0xF8;
		*ptr |= src->mem.base & 7;
		ptr++;

		if(src->needsRex && src->mem.base > 7)
			*rexPtr |= 0b0001;

		return ptr - startPtr + jitas_placeArg(ptr, ins->destination, dst);
	}
	else if(ins->destination == JITAS_ARG_REG)
	{
		ptr--;
		*ptr &= 0xF8;
		*ptr |= dst->mem.base & 0b111;
		ptr++;

		if(dst->needsRex && dst->mem.base > 7)
			*rexPtr |= 0b0001;

		return ptr - startPtr + jitas_placeArg(ptr, ins->source, src);
	}
	else
	{
		int size = ptr - startPtr;
		size += jitas_placeArg(ptr, ins->source, src);
		size += jitas_placeArg(ptr, ins->destination, dst);

		return size;
	}
}
