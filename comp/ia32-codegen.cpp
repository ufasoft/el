#include <el/ext.h>

#include "ia32-codegen.h"

namespace Ia32 {

void Ia32Codegen::Emit(Ia32Instr instr, const Ia32Op& op, const Ia32Op& op2) {
	switch (instr) {
		case Ia32Instr::INSTR_MOV:
			{
				if (op.Type == Ia32Op::OP_REG && op.BaseReg==REG_A_CX && op2.Type == Ia32Op::OP_OFFSET) {
					EmitByte(0xB9);
					EmitInt32(op2.Offset);
				} else
					Throw(E_NOTIMPL);
			}
			break;
		case Ia32Instr::INSTR_CALL:
			switch (op.BaseReg & ~REG_MASK) {
				case REG_GP:
					EmitByte(0xFF);
					EmitByte(0xD0 | (op.BaseReg & REG_MASK));
					break;
			}
			break;
		case Ia32Instr::INSTR_RET:
			if (op.Type == Ia32Op::OP_NONE)
				EmitByte(0xC3);
			else {
				EmitByte(0xC2);
				EmitInt16((int16_t)op.Offset);
			}
			break;
		case Ia32Instr::INSTR_IRET:	EmitByte(0xCF); break;
		case Ia32Instr::INSTR_PUSH:
			if ((op.BaseReg & ~REG_MASK) != REG_GP)
				Throw(E_FAIL);
			EmitByte(0x50 |  (op.BaseReg & REG_MASK));
			break;
	}
}

} // namespace Ia32