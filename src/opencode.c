#include <stdlib.h>

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

static void jitas_placeArg(jitas_context_t *ctx, jitas_instruction_t *ins, jitas_argtype_t opArg, jitas_argument_t *arg)
{
	if(opArg == JITAS_ARG_IMM8)
	{
		*ctx->ptr++ = arg->imm;
	}
	else if(opArg == JITAS_ARG_IMM16)
	{
		*(int16_t *)ctx->ptr = arg->imm;
		ctx->ptr += 2;
	}
	else if(opArg == JITAS_ARG_REL8 || opArg == JITAS_ARG_REL32)
	{
		jitas_symboltable_t *entry = malloc(sizeof(jitas_symboltable_t));
		entry->size = opArg == JITAS_ARG_REL8 ? 1 : 4;
		entry->line = ctx->line;
		entry->symbol = arg->symbol;
		entry->nextInsPtr = ctx->ptr + entry->size; //TODO is this correct for all instructions?
		entry->ptr = ctx->ptr;
		entry->next = ctx->symbols;
		ctx->symbols = entry;

		if(opArg == JITAS_ARG_REL8)
			*ctx->ptr = 0;
		else
			*(int32_t *)ctx->ptr = 0;
		ctx->ptr += entry->size;
	}
	else if(arg->type == JITAS_ARG_SYMBOL_ADDRESS)
	{
		jitas_symboltable_t *entry = malloc(sizeof(jitas_symboltable_t));
		entry->size = sizeof(void *);
		entry->line = ctx->line;
		entry->symbol = arg->symbol;
		entry->nextInsPtr = ctx->ptr + sizeof(void *);
		entry->ptr = ctx->ptr;
		entry->next = ctx->symbols;
		ctx->symbols = entry;

		*(void **)ctx->ptr = 0;
		ctx->ptr += sizeof(void *);
	}
	else if(opArg == JITAS_ARG_IMM || opArg == JITAS_ARG_IMM_MAX32)
	{
		int size = arg->size > 4 && opArg == JITAS_ARG_IMM_MAX32 ? 4 : arg->size;
		if(size == 0)
		{
			switch(ins->size)
			{
				case JITAS_SIZE_BYTE:
					size = 1;
					break;
				case JITAS_SIZE_ANY:
					size = 4; //TODO cause an error here instead?
					break;
				case JITAS_SIZE_PTR:
					size = 8;
					break;
				case JITAS_SIZE_IGNORE:
					//TODO cause an error here
					break;
			}
		}

		switch(size)
		{
			case 1:
				*ctx->ptr = arg->imm;
				break;
			case 2:
				*(int16_t *)ctx->ptr = arg->imm;
				break;
			case 4:
				*(int32_t *)ctx->ptr = arg->imm;
				break;
			case 8:
				*(int64_t *)ctx->ptr = arg->imm;
				break;
		}
		ctx->ptr += size;
	}
}

void jitas_encode(jitas_context_t *ctx, jitas_instruction_t *ins, jitas_argument_t *src, jitas_argument_t *dst)
{
	if(ins->size == JITAS_SIZE_ANY && (src->size == 2 || dst->size == 2))
		*ctx->ptr++ = 0x66;

	uint8_t *rexPtr = ctx->ptr;
	if(src->needsRex || dst->needsRex || (ins->size == JITAS_SIZE_ANY && (src->size == 8 || dst->size == 8)))
	{
		*ctx->ptr = 0x40;
		if(ins->size == JITAS_SIZE_ANY && (src->size == 8 || dst->size == 8))
			*ctx->ptr |= 0b1000;
		ctx->ptr++;
	}

	for(int i = 0; i < ins->oplen; i++)
		*ctx->ptr++ = ins->opcode[i];

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

		if(dst->type == JITAS_ARG_SYMBOL)
		{
			modrm.mod = 0b00;
			modrm.rm = 5;
			*ctx->ptr++ = *(uint8_t *)&modrm;

			jitas_symboltable_t *entry = malloc(sizeof(jitas_symboltable_t));
			entry->size = 4;
			entry->line = ctx->line;
			entry->symbol = dst->symbol;
			entry->ptr = ctx->ptr;
			entry->next = ctx->symbols;
			ctx->symbols = entry;

			*(int32_t *)ctx->ptr = 0;
			ctx->ptr += 4;

			if(srcType != JITAS_ARG_REG)
				jitas_placeArg(ctx, ins, srcType, src);

			entry->nextInsPtr = ctx->ptr;
		}
		else
		{
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

			if(dst->mem.scale == 0 || (dst->type == JITAS_ARG_MODRM && (dst->mem.base & 7) != 4))
			{
				modrm.rm = dst->mem.base & 7;
				*ctx->ptr++ = *(uint8_t *)&modrm;
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

				*ctx->ptr++ = *(uint8_t *)&modrm;
				*ctx->ptr++ = *(uint8_t *)&sib;
			}

			switch(modrm.mod)
			{
				case 0b01:
					*(int8_t *)ctx->ptr++ = dst->mem.offset;
					break;
				case 0b10:
					*(int32_t *)ctx->ptr = dst->mem.offset;
					ctx->ptr += 4;
					break;
			}

			if(srcType != JITAS_ARG_REG)
				jitas_placeArg(ctx, ins, srcType, src);
		}
	}
	else if(ins->source == JITAS_ARG_REG)
	{
		ctx->ptr--;
		*ctx->ptr &= 0xF8;
		*ctx->ptr |= src->mem.base & 7;
		ctx->ptr++;

		if(src->needsRex && src->mem.base > 7)
			*rexPtr |= 0b0001;

		jitas_placeArg(ctx, ins, ins->destination, dst);
	}
	else if(ins->destination == JITAS_ARG_REG)
	{
		ctx->ptr--;
		*ctx->ptr &= 0xF8;
		*ctx->ptr |= dst->mem.base & 0b111;
		ctx->ptr++;

		if(dst->needsRex && dst->mem.base > 7)
			*rexPtr |= 0b0001;

		jitas_placeArg(ctx, ins, ins->source, src);
	}
	else
	{
		jitas_placeArg(ctx, ins, ins->source, src);
		jitas_placeArg(ctx, ins, ins->destination, dst);
	}
}
