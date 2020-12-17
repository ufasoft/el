#pragma once

//!!!#include <ia32.h>

namespace Ia32 {

ENUM_CLASS(Ia32Instr) {
	INSTR_MOV,
	INSTR_CALL,
	INSTR_RET,
	INSTR_IRET,
	INSTR_PUSH
} END_ENUM_CLASS(Ia32Instr);


const byte REG_MASK = 0x07,
			REG_GP	= 0x08;


enum Ia32Register {
	REG_A_AX = 0 | REG_GP,
	REG_A_CX = 1 | REG_GP,
};


class Ia32Op {
public:
	enum EType {
		OP_NONE,
		OP_REG,
		OP_OFFSET
	};

	enum EType Type;

	Ia32Register BaseReg;
	int32_t Offset;
	CBool Indirect;

	Ia32Op()
		: Type(OP_NONE)
	{
	}

	Ia32Op(Ia32Register reg)
		:	Type(OP_REG)
		,	BaseReg(reg)
	{
	}

	Ia32Op(int32_t off)
		:	Type(OP_OFFSET)
		,	Offset(off)
	{
	}

	Ia32Op(void *off)
		:	Type(OP_OFFSET)
		,	Offset((int32_t)(int64_t)off)
	{
	}
};




class Ia32Codegen {
public:
	BinaryWriter Wr;

	Ia32Codegen(class Stream& stm)
		:	Wr(stm)
	{
	}

	void EmitByte(byte b) {
		Wr << b;
	}

	void EmitInt16(int16_t v) {
		Wr << v;
	}

	void EmitInt32(int32_t v) {
		Wr << v;
	}

//!!!R	void Emit(Ia32Instr instr);
	void Emit(Ia32Instr instr, const Ia32Op& op = Ia32Op(), const Ia32Op& op2 = Ia32Op());

	Ia32Op operator[](Ia32Op op) {
		op.Indirect = true;
		return op;
	}
};

} // namespace Ia32














