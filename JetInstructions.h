#ifndef _JET_INSTRUCTIONS_HEADER
#define _JET_INSTRUCTIONS_HEADER

namespace Jet
{
	static enum InstructionType
	{
		Add,
		Mul,
		Div,
		Sub,
		Modulus,

		Negate,

		BAnd,
		BOr,

		Eq,
		NotEq,
		Lt,
		Gt,
		LtE,
		GtE,

		Incr,
		Decr,

		Dup,
		Pop,

		LdNum,
		LdNull,
		LdStr,
		LoadFunction,

		Jump,
		JumpTrue,
		JumpFalse,

		NewArray,
		NewObject,

		Store,
		Load,
		//local vars
		LStore,
		LLoad,

		Close,//closes a range of locals

		//index functions
		LoadAt,
		StoreAt,

		//these all work on the last value in the stack
		EStore,
		ELoad,
		ECall,

		Call,
		Return,

		//dummy instructions for the assembler/debugging
		Label,
		Comment,
		DebugLine,
		Function
	};
}

#endif