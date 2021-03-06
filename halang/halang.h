#pragma once
#include <cinttypes>

namespace halang
{
	typedef int32_t uc32;

	// runtime definition
	typedef double TNumber;
	typedef int TSmallInt;
	typedef bool TBool;

#define VM_STACK_SIZE 256

#define OPERATOR_LIST(V) \
	V(ADD, "+", 10) \
	V(SUB, "-", 10) \
	V(MUL, "*", 11) \
	V(DIV, "/", 11) \
	V(MOD, "%", 12) \
	V(POW, "**", 13) \
	V(NOT, "!", 15) \
	V(EQ, "==", 8) \
	V(GT, ">", 8) \
	V(LT, "<", 8) \
	V(GTEQ, ">=", 8) \
	V(LTEQ, "<=", 8) \
	V(AND, "&&", 5) \
	V(OR, "||", 5) \
	V(ILLEGAL_OP, "", 0)

#define TOKEN_LIST(V) \
	V(GET) \
	V(SET) \
	V(GETTER) \
	V(SETTER) \
	V(ACCESSOR) \
	V(DOT) \
	V(AT) \
	V(COMMA) \
	V(SEMICOLON) \
	V(ILLEGAL) \
	V(IF) \
	V(ELSE) \
	V(WHILE) \
	V(BREAK) \
	V(CONTINUE) \
	V(ASSIGN) \
	V(IDENTIFIER) \
	V(STRING) \
	V(OPEN_PAREN) \
	V(CLOSE_PAREN) \
	V(OPEN_BRAKET) \
	V(CLOSE_BRAKET) \
	V(OPEN_SQUARE_BRAKET) \
	V(CLOSE_SQUARE_BRAKET) \
	V(NUMBER) \
	V(VAR) \
	V(CLASS) \
	V(FUNC) \
	V(RETURN) \
	V(ENDFILE) 


#define E(NAME, STR, PRE) NAME,
	enum OperatorType
	{
		OPERATOR_LIST(E)
		OP_NUM
	};
#undef E


#define E(NAME, STR, PRE) PRE,
	const int OpreatorPrecedence[OperatorType::OP_NUM] =
	{
		OPERATOR_LIST(E)
	};
#undef E

#define E(NAME, STR, PRE) #NAME ,
#define ET(NAME) #NAME ,
	const static char* TokenName[] =
	{
		OPERATOR_LIST(E)
		TOKEN_LIST(ET)
	};
#undef E
#undef ET

	inline int getPrecedence(OperatorType ot)
	{
		if (ot >= OperatorType::OP_NUM || ot < 0) return 0;
		return OpreatorPrecedence[ot];
	}

};
