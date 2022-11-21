// A sister translation file of AST.cpp, moved over because this is very long & repetitive and clogs up AST.cpp a whole bunch
#include "Forward.h"
#include "AST.h"
#include "Object.h"
#include "Table.h"
#include "Interpreter.h"
#include "Parser.h"

#define BIN_ENUMS(a,b,c) ( (uint32_t(a) << 16) | (uint32_t(b) << 8)  | uint32_t(c) )
#define EXOR(a,b) ((a ? !(b) : b))

#ifdef __has_builtin
#if __has_builtin (__builtin_bit_cast) // Making use of the fact that this compiler probably has C++20 spec internally
#define DND(op) return __builtin_bit_cast(uint64_t,lhs.t_value.as_double) op __builtin_bit_cast(uint64_t,rhs.t_value.as_double)
#define DNI(op) return __builtin_bit_cast(uint64_t,lhs.t_value.as_double) op __builtin_bit_cast(uint64_t,static_cast<int64_t>(rhs.t_value.as_int))
#define IND(op) return __builtin_bit_cast(uint64_t,static_cast<int64_t>(lhs.t_value.as_int)) op __builtin_bit_cast(uint64_t,rhs.t_value.as_double)
#endif
#endif
#ifndef DND // Horrifying fallback implementation
/* WARNING: Chaotically-aligned programming */
// For doing bitwise on double-with-double. Wanted to use templates but I don't think you conveniently can in this odd instance.
#define DND(op) \
{ \
	const unsigned char bigness = sizeof(lhs.t_value.as_double); \
	unsigned char* alpha = reinterpret_cast<unsigned char*>(&lhs.t_value.as_double); \
	unsigned char* beta = reinterpret_cast<unsigned char*>(&rhs.t_value.as_double); \
	unsigned char charlie[bigness]; \
	for (char i = 0; i < bigness; ++i) \
	{ \
		charlie[i] = alpha[i] op beta[i]; \
	} \
	return Value(*reinterpret_cast<double*>(charlie)); \
} \

// For doing bitwise on double-with-int.
// Note that it tries to be careful about handling the case where JoaoInts are defined to be 32-bit integers instead of 64.
#define DNI(op) \
{ \
	const unsigned char bigness = sizeof(lhs.t_value.as_double); \
	unsigned char* alpha = reinterpret_cast<unsigned char*>(&lhs.t_value.as_double); \
	unsigned char* beta = reinterpret_cast<unsigned char*>(&rhs.t_value.as_int); \
	unsigned char charlie[bigness]; \
	if(std::is_same<Value::JoaoInt,int32_t>::value) \
	{ \
		for(char i = 0; i < 4; ++i)\
		{\
			charlie[i] = alpha[i] op beta[i];\
		}\
		for (char j = 4; j < bigness; ++j)\
		{\
			charlie[j] = alpha[j] op 0;\
		}\
	} \
	else \
	{ \
		for (char i = 0; i < bigness; ++i) \
		{ \
			charlie[i] = alpha[i] op beta[i]; \
		} \
	}\
	return Value(*reinterpret_cast<double*>(charlie)); \
} \

// For doing bitwise on int-with-double.
// Note that it tries to be careful about handling the case where JoaoInts are defined to be 32-bit integers instead of 64.
#define IND(op) \
{ \
	const unsigned char bigness = sizeof(rhs.t_value.as_double); \
	unsigned char* alpha = reinterpret_cast<unsigned char*>(&lhs.t_value.as_int); \
	unsigned char* beta = reinterpret_cast<unsigned char*>(&rhs.t_value.as_double); \
	unsigned char charlie[bigness]; \
	if(std::is_same<Value::JoaoInt,int32_t>::value) \
	{ \
		for(char i = 0; i < 4; ++i)\
		{\
			charlie[i] = alpha[i] op beta[i];\
		}\
	} \
	else \
	{ \
		for (char i = 0; i < bigness; ++i) \
		{ \
			charlie[i] = alpha[i] op beta[i]; \
		} \
	}\
	return Value(*reinterpret_cast<double*>(charlie)); \
}
#endif


Value BinaryExpression::BinaryOperation(const Value& lhs, const Value& rhs, BinaryExpression::bOps t_op, Interpreter& interp)
{
	Value::vType lhs_type = lhs.t_vType;
	Value::vType rhs_type = rhs.t_vType;

	/*
	So for this we're going to do something a little wacky.
	I want this to be only one switch statement, but there's three dimensions of combinatorics:
	the operator used, the type of lhs, and the type of rhs.
	To do this, I am going to synthesize these three UInt8 enums into one Uint32 that is then switched against to find what to do.
	*/

	uint32_t switcher = BIN_ENUMS(t_op, lhs_type, rhs_type);

	switch (switcher)
	{
	//DOUBLE & DOUBLE
	case(BIN_ENUMS(bOps::Add, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double + rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double - rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double * rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double / rhs.t_value.as_double);
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Double, Value::vType::Double)):
		return Value(floor(lhs.t_value.as_double / rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Double, Value::vType::Double)):
		return Value(pow(lhs.t_value.as_double, rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Double, Value::vType::Double)):
	{
		double nowhere;
		return Value(modf(lhs.t_value.as_double / rhs.t_value.as_double, &nowhere) * rhs.t_value.as_double);
	}
	//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Double, Value::vType::Double)): // WARNING: Chaotically-aligned programming
		DND(&);
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Double, Value::vType::Double)): 
		DND(^);
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Double, Value::vType::Double)):
		DND(|);
	//
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Double, Value::vType::Double)):
	{
		uint64_t chaos = *reinterpret_cast<const double*>(&lhs.t_value.as_double);
		return Value(static_cast<int64_t>(chaos >> Value::JoaoInt(rhs.t_value.as_double)));
	}
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Double, Value::vType::Double)):
	{
		uint64_t chaos = *reinterpret_cast<const double*>(&lhs.t_value.as_double);
		return Value(static_cast<int64_t>(chaos << Value::JoaoInt(rhs.t_value.as_double)));
	}
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Double, Value::vType::Double)):
		return Value(std::to_string(lhs.t_value.as_double) + std::to_string(rhs.t_value.as_double));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double < rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double <= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double > rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double >= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double == rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double != rhs.t_value.as_double);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double && rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double || rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Double, Value::vType::Double)):
		return Value(EXOR(lhs.t_value.as_double,rhs.t_value.as_double));


	//DOUBLE & INT
	case(BIN_ENUMS(bOps::Add, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double + rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double - rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double * rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double / rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Double, Value::vType::Integer)):
		return Value(floor(lhs.t_value.as_double / rhs.t_value.as_int));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Double, Value::vType::Integer)):
		return Value(pow(lhs.t_value.as_double, rhs.t_value.as_int));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Double, Value::vType::Integer)):
	{
		double nowhere;
		return Value(modf(lhs.t_value.as_double / rhs.t_value.as_int, &nowhere) * rhs.t_value.as_int);
	}
	//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Double, Value::vType::Integer)): // WARNING: Chaotically-aligned programming
		DNI(&);
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Double, Value::vType::Integer)):
		DNI(^);
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Double, Value::vType::Integer)):
		DNI(|);
	//
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Double, Value::vType::Integer)):
	{
		uint64_t chaos = *reinterpret_cast<const double*>(&lhs.t_value.as_double);
		return Value(static_cast<int64_t>(chaos >> rhs.t_value.as_int));
	}
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Double, Value::vType::Integer)):
	{
		uint64_t chaos = *reinterpret_cast<const double*>(&lhs.t_value.as_double);
		return Value(static_cast<int64_t>(chaos << rhs.t_value.as_int));
	}
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Double, Value::vType::Integer)):
		return Value(std::to_string(lhs.t_value.as_double) + std::to_string(rhs.t_value.as_int));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double < rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double <= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double > rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double >= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double == rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double != rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double && rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double || rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Double, Value::vType::Integer)):
		return Value(EXOR(lhs.t_value.as_double,rhs.t_value.as_int));


	//INT & DOUBLE
	case(BIN_ENUMS(bOps::Add, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int + rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int - rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int * rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int / rhs.t_value.as_double);
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Integer, Value::vType::Double)):
		return Value(floor(lhs.t_value.as_int / rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Integer, Value::vType::Double)):
		return Value(pow(lhs.t_value.as_int, rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Integer, Value::vType::Double)):
	{
		double dummy;
		return Value(modf(lhs.t_value.as_int / rhs.t_value.as_double, &dummy));
	}
	//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Integer, Value::vType::Double)): // WARNING: Chaotically-aligned programming
		IND(&);
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Integer, Value::vType::Double)):
		IND(^);
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Integer, Value::vType::Double)):
		IND(|);
	//
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Integer, Value::vType::Double)):
		return(Value(lhs.t_value.as_int << Value::JoaoInt(lhs.t_value.as_double)));
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Integer, Value::vType::Double)):
		return(Value(lhs.t_value.as_int >> Value::JoaoInt(lhs.t_value.as_double)));
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Integer, Value::vType::Double)):
		return Value(std::to_string(lhs.t_value.as_int) + std::to_string(rhs.t_value.as_double));
		//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int < rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int <= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int > rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int >= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int == rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int != rhs.t_value.as_double);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int && rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int || rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Integer, Value::vType::Double)):
		return Value(EXOR(lhs.t_value.as_int,rhs.t_value.as_double));


	//INT & INT
	case(BIN_ENUMS(bOps::Add, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int + rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int - rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int * rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Integer, Value::vType::Integer)): // Intentionally cascades to FloorDivide
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Integer, Value::vType::Integer)): // :)
		return Value(lhs.t_value.as_int / rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Integer, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(pow(lhs.t_value.as_int, rhs.t_value.as_int)));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int % rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int & rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int ^ rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int | rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int >> rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int << rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Integer, Value::vType::Integer)):
		return Value(std::to_string(lhs.t_value.as_int) + std::to_string(rhs.t_value.as_int));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int < rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int <= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int > rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int >= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int == rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int != rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int && rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int || rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Integer, Value::vType::Integer)):
		return Value(EXOR(lhs.t_value.as_int,rhs.t_value.as_int));


	//BOOL & BOOL
	case(BIN_ENUMS(bOps::Add, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) + static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) - static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) * static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Divide, Value::vType::Bool, Value::vType::Bool)): // Intentionally cascades to FloorDivide
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Bool, Value::vType::Bool)): // :)
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) / static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(pow(static_cast<Value::JoaoInt>(lhs.t_value.as_bool), static_cast<Value::JoaoInt>(rhs.t_value.as_bool))));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) % static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) & static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) ^ static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) | static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) >> static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Bool, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) << static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Bool, Value::vType::Bool)):
		return Value(std::to_string(lhs.t_value.as_bool) + std::to_string(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Bool, Value::vType::Bool)):
		return Value(lhs.t_value.as_bool < rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Bool, Value::vType::Bool)):
		return Value(lhs.t_value.as_bool <= rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Bool, Value::vType::Bool)):
		return Value(lhs.t_value.as_bool > rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Bool, Value::vType::Bool)):
		return Value(lhs.t_value.as_bool >= rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Bool, Value::vType::Bool)):
		return Value(lhs.t_value.as_bool == rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Bool, Value::vType::Bool)):
		return Value(lhs.t_value.as_bool != rhs.t_value.as_bool);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Bool, Value::vType::Bool)):
		return Value(lhs.t_value.as_bool && rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Bool, Value::vType::Bool)):
		return Value(lhs.t_value.as_bool || rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Bool, Value::vType::Bool)):
		return Value(EXOR(lhs.t_value.as_bool,rhs.t_value.as_bool));


	//BOOL & DOUBLE
	case(BIN_ENUMS(bOps::Add, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) + rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) - rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) * rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) / rhs.t_value.as_double);
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Bool, Value::vType::Double)):
		return Value(floor(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) / rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Bool, Value::vType::Double)):
		return Value(pow(static_cast<Value::JoaoInt>(lhs.t_value.as_bool), rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Bool, Value::vType::Double)):
	{
		double dummy;
		return Value(modf(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) / rhs.t_value.as_double, &dummy));
	}
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Bool, Value::vType::Double)):
		return Value(std::to_string(lhs.t_value.as_bool) + std::to_string(rhs.t_value.as_double));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) < rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) <= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) > rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) >= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) == rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Bool, Value::vType::Double)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) != rhs.t_value.as_double);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Bool, Value::vType::Double)):
		return Value(lhs.t_value.as_bool && rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Bool, Value::vType::Double)):
		return Value(lhs.t_value.as_bool || rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Bool, Value::vType::Double)):
		return Value((lhs.t_value.as_bool || rhs.t_value.as_double) && !(lhs.t_value.as_bool && rhs.t_value.as_double));

	//DOUBLE & BOOL
	case(BIN_ENUMS(bOps::Add, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double + static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double - static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double * static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Divide, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double / static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Double, Value::vType::Bool)):
		return Value(floor(lhs.t_value.as_double / static_cast<Value::JoaoInt>(rhs.t_value.as_bool)));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Double, Value::vType::Bool)):
		return Value(pow(lhs.t_value.as_double, static_cast<Value::JoaoInt>(rhs.t_value.as_bool)));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Double, Value::vType::Bool)):
	{
		double nowhere;
		if (rhs.t_value.as_bool)
		{
			return Value(modf(lhs.t_value.as_double,&nowhere));
		}
		interp.RuntimeError(nullptr, ErrorCode::FailedOperation, "Division by zero!");
		return Value();
	}
	//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Double, Value::vType::Bool)): // WARNING: Chaotically-aligned programming
		if (rhs.t_value.as_bool)
		{
			if (reinterpret_cast<const unsigned char*>(&lhs.t_value.as_double)[0] & 0b1)
			{
				return(Value(5.0e-324));
			}
		}
		return Value(double(0));
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Double, Value::vType::Bool)):
		//If false, do nothing
		//If true, flip the last bit
	{
		if (rhs.t_value.as_bool)
		{
			double trouble = lhs.t_value.as_double;
			unsigned char* alpha = reinterpret_cast<unsigned char*>(&trouble);
			alpha[0] = alpha[0] ^ (static_cast<unsigned char>(0b1));
			return Value(trouble);
		}
		else
		{
			return Value(lhs.t_value.as_double);
		}
	}
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Double, Value::vType::Bool)):
	{
		double trouble = lhs.t_value.as_double;
		unsigned char* alpha = reinterpret_cast<unsigned char*>(&trouble);
		alpha[0] = alpha[0] | static_cast<unsigned char>(rhs.t_value.as_bool);
		return Value(trouble);
	}
	//
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Double, Value::vType::Bool)):
	{
		uint64_t chaos = *reinterpret_cast<const double*>(&lhs.t_value.as_double);
		return Value(static_cast<int64_t>(chaos >> static_cast<Value::JoaoInt>(rhs.t_value.as_bool)));
	}
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Double, Value::vType::Bool)):
	{
		uint64_t chaos = *reinterpret_cast<const double*>(&lhs.t_value.as_double);
		return Value(static_cast<int64_t>(chaos << static_cast<Value::JoaoInt>(rhs.t_value.as_bool)));
	}
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Double, Value::vType::Bool)):
		return Value(std::to_string(lhs.t_value.as_double) + std::to_string(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double < static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double <= static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Greater, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double > static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double >= static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Equals, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double == static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double != static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double && rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Double, Value::vType::Bool)):
		return Value(lhs.t_value.as_double || rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Double, Value::vType::Bool)):
		return Value(EXOR(lhs.t_value.as_double, rhs.t_value.as_bool));


	//BOOL & INT
	case(BIN_ENUMS(bOps::Add, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool + rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool - rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool * rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Bool, Value::vType::Integer)): // Intentionally cascades to FloorDivide
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Bool, Value::vType::Integer)): // :)
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) / rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Bool, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(pow(lhs.t_value.as_bool, rhs.t_value.as_int)));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Bool, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) % rhs.t_value.as_int);
		//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool & rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool ^ rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool | rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool >> rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool << rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Bool, Value::vType::Integer)):
		return Value(std::to_string(lhs.t_value.as_bool) + std::to_string(rhs.t_value.as_int));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Bool, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) < rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Bool, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) <= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Bool, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) > rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Bool, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) >= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Bool, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) == rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Bool, Value::vType::Integer)):
		return Value(static_cast<Value::JoaoInt>(lhs.t_value.as_bool) != rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool && rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Bool, Value::vType::Integer)):
		return Value(lhs.t_value.as_bool || rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Bool, Value::vType::Integer)):
		return Value(EXOR(lhs.t_value.as_bool, rhs.t_value.as_int));

	//INT & BOOL
	case(BIN_ENUMS(bOps::Add, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int + rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int - rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int * rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Integer, Value::vType::Bool)): // Intentionally cascades to FloorDivide
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Integer, Value::vType::Bool)): // :)
		return Value(lhs.t_value.as_int / static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Integer, Value::vType::Bool)):
		return Value(static_cast<Value::JoaoInt>(pow(lhs.t_value.as_int, rhs.t_value.as_bool)));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int % static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int & static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int ^ static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int | static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int >> static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int << static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Integer, Value::vType::Bool)):
		return Value(std::to_string(lhs.t_value.as_int) + std::to_string(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int < static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int <= static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Greater, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int > static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int >= static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::Equals, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int == static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int != static_cast<Value::JoaoInt>(rhs.t_value.as_bool));
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int && rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Integer, Value::vType::Bool)):
		return Value(lhs.t_value.as_int || rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Integer, Value::vType::Bool)):
		return Value(EXOR(lhs.t_value.as_int, rhs.t_value.as_bool));

	//STRING & STRING
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::String, Value::vType::String)):
	{
		std::string newstr = *(lhs.t_value.as_string_ptr) + *(rhs.t_value.as_string_ptr);
		return Value(newstr);
	}
	//
	case(BIN_ENUMS(bOps::Equals, Value::vType::String, Value::vType::String)):
	{
		return Value(*(lhs.t_value.as_string_ptr) == *(rhs.t_value.as_string_ptr));
	}
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::String, Value::vType::String)):
	{
		return Value(*(lhs.t_value.as_string_ptr) != *(rhs.t_value.as_string_ptr));
	}
	case(BIN_ENUMS(bOps::Greater, Value::vType::String, Value::vType::String)):
	{
		return Value(*(lhs.t_value.as_string_ptr) > *(rhs.t_value.as_string_ptr));
	}
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::String, Value::vType::String)):
	{
		return Value(*(lhs.t_value.as_string_ptr) >= *(rhs.t_value.as_string_ptr));
	}
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::String, Value::vType::String)):
	{
		return Value(*(lhs.t_value.as_string_ptr) <= *(rhs.t_value.as_string_ptr));
	}
	case(BIN_ENUMS(bOps::LessThan, Value::vType::String, Value::vType::String)):
	{
		return Value(*(lhs.t_value.as_string_ptr) < *(rhs.t_value.as_string_ptr));
	}
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::String, Value::vType::String)):
	case(BIN_ENUMS(bOps::LogicalOr,  Value::vType::String, Value::vType::String)):
		return Value(true);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::String, Value::vType::String)):
		return Value(false);

	//STRING & DOUBLE
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::String, Value::vType::Double)):
	{
		std::string newstr = *(lhs.t_value.as_string_ptr) + std::to_string(rhs.t_value.as_double);
		return Value(newstr);
	}
	//
	case(BIN_ENUMS(bOps::Equals, Value::vType::String, Value::vType::Double)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::String, Value::vType::Double)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::String, Value::vType::Double)):
		return Value(true);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::String, Value::vType::Double)):
		return Value(rhs.t_value.as_double != static_cast<double>(0));
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::String, Value::vType::Double)):
		return Value(rhs.t_value.as_double == static_cast<double>(0));

	//DOUBLE & STRING
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Double, Value::vType::String)):
	{
		std::string newstr = std::to_string(lhs.t_value.as_double) + *(rhs.t_value.as_string_ptr);
		return Value(newstr);
	}
	//
	case(BIN_ENUMS(bOps::Equals, Value::vType::Double, Value::vType::String)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Double, Value::vType::String)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Double, Value::vType::String)):
		return Value(true);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Double, Value::vType::String)):
		return Value(lhs.t_value.as_double != static_cast<double>(0));
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Double, Value::vType::String)):
		return Value(lhs.t_value.as_double == static_cast<double>(0));

	//STRING & INT
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::String, Value::vType::Integer)):
	{
		std::string newstr = *(lhs.t_value.as_string_ptr) + std::to_string(rhs.t_value.as_int);
		return Value(newstr);
	}
	//
	case(BIN_ENUMS(bOps::Equals, Value::vType::String, Value::vType::Integer)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::String, Value::vType::Integer)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::String, Value::vType::Integer)):
		return Value(true);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::String, Value::vType::Integer)):
		return Value(rhs.t_value.as_int != 0);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::String, Value::vType::Integer)):
		return Value(rhs.t_value.as_int == 0);

	//INT & STRING
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Integer, Value::vType::String)):
	{
		std::string newstr = std::to_string(lhs.t_value.as_int) + *(rhs.t_value.as_string_ptr);
		return Value(newstr);
	}
	//
	case(BIN_ENUMS(bOps::Equals, Value::vType::Integer, Value::vType::String)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Integer, Value::vType::String)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Integer, Value::vType::String)):
		return Value(true);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Integer, Value::vType::String)):
		return Value(lhs.t_value.as_int != 0);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Integer, Value::vType::String)):
		return Value(lhs.t_value.as_int == 0);

	//STRING & BOOL
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::String, Value::vType::Bool)):
	{
		std::string newstr = *(lhs.t_value.as_string_ptr) + std::to_string(rhs.t_value.as_bool);
		return Value(newstr);
	}
	//
	case(BIN_ENUMS(bOps::Equals, Value::vType::String, Value::vType::Bool)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::String, Value::vType::Bool)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::String, Value::vType::Bool)):
		return Value(true);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::String, Value::vType::Bool)):
		return Value(rhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::String, Value::vType::Bool)):
		return Value(!rhs.t_value.as_bool);

	//BOOL & STRING
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Bool, Value::vType::String)):
	{
		std::string newstr = std::to_string(lhs.t_value.as_bool) + *(rhs.t_value.as_string_ptr);
		return Value(newstr);
	}
	//
	case(BIN_ENUMS(bOps::Equals, Value::vType::Bool, Value::vType::String)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Bool, Value::vType::String)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Bool, Value::vType::String)):
		return Value(true);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Bool, Value::vType::String)):
		return Value(lhs.t_value.as_bool);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Bool, Value::vType::String)):
		return Value(!lhs.t_value.as_bool);


	//NULL & NULL
	case(BIN_ENUMS(bOps::Equals, Value::vType::Null, Value::vType::Null)):
		return Value(true);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Null, Value::vType::Null)):
		return Value(false);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Null, Value::vType::Null)):
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Null, Value::vType::Null)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Null, Value::vType::Null)):
		return Value(false);
	

	//NULL & DOUBLE
	case(BIN_ENUMS(bOps::Equals, Value::vType::Null, Value::vType::Double)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Null, Value::vType::Double)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Null, Value::vType::Double)):
		return Value(false);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Null, Value::vType::Double)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Null, Value::vType::Double)):
		return Value(rhs.t_value.as_double != static_cast<double>(0));


	//DOUBLE & NULL
	case(BIN_ENUMS(bOps::Equals, Value::vType::Double, Value::vType::Null)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Double, Value::vType::Null)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Double, Value::vType::Null)):
		return Value(false);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Double, Value::vType::Null)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Double, Value::vType::Null)):
		return Value(lhs.t_value.as_double != static_cast<double>(0));


	//NULL & INT
	case(BIN_ENUMS(bOps::Equals, Value::vType::Null, Value::vType::Integer)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Null, Value::vType::Integer)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Null, Value::vType::Integer)):
		return Value(false);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Null, Value::vType::Integer)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Null, Value::vType::Integer)):
		return Value(rhs.t_value.as_int != 0);


	//INT & NULL
	case(BIN_ENUMS(bOps::Equals, Value::vType::Integer, Value::vType::Null)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Integer, Value::vType::Null)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Integer, Value::vType::Null)):
		return Value(false);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Integer, Value::vType::Null)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Integer, Value::vType::Null)):
		return Value(lhs.t_value.as_int != 0);


	//NULL & BOOL
	case(BIN_ENUMS(bOps::Equals, Value::vType::Null, Value::vType::Bool)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Null, Value::vType::Bool)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Null, Value::vType::Bool)):
		return Value(false);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Null, Value::vType::Bool)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Null, Value::vType::Bool)):
		return Value(rhs.t_value.as_bool);


	//BOOL & NULL
	case(BIN_ENUMS(bOps::Equals, Value::vType::Bool, Value::vType::Null)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Bool, Value::vType::Null)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Bool, Value::vType::Null)):
		return Value(false);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Bool, Value::vType::Null)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Bool, Value::vType::Null)):
		return Value(lhs.t_value.as_bool);


	//NULL & STRING
	case(BIN_ENUMS(bOps::Equals, Value::vType::Null, Value::vType::String)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Null, Value::vType::String)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Null, Value::vType::String)):
		return Value(false);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Null, Value::vType::String)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Null, Value::vType::String)):
		return Value(true);

	//STRING & NULL
	case(BIN_ENUMS(bOps::Equals, Value::vType::String, Value::vType::Null)):
		return Value(false);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::String, Value::vType::Null)):
		return Value(true);
	//
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::String, Value::vType::Null)):
		return Value(false);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::String, Value::vType::Null)):
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::String, Value::vType::Null)):
		return Value(true);

	/*
	----------------------------------------------------------------------------
	OVERLOADABLE OPERATORS
	----------------------------------------------------------------------------
	*/
	//OBJECT & OBJECT
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Object, Value::vType::Object)):
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Object, Value::vType::Object)):
		return Value(true);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Object, Value::vType::Object)):
		return Value(false);
	default:
		switch (t_op) // Behold, a shitty kludge to handle rawdog-casting Objects to boolean true in logical expressions
		{
		case(BinaryExpression::bOps::LogicalAnd):
		case(BinaryExpression::bOps::LogicalOr):
		case(BinaryExpression::bOps::LogicalXor):
			if (lhs.t_vType == Value::vType::Object)
			{
				return Value(true && rhs);
			}
			else if (rhs.t_vType == Value::vType::Object)
			{
				return Value(lhs && true);
			}
		default:
			interp.RuntimeError(nullptr, ErrorCode::FailedOperation, "Failed to do a binary operation! (" + lhs.to_string() + ", " + rhs.to_string() + ")\nTypes: (" + lhs.typestring() + ", " + rhs.typestring() + ")");
			return Value();
		}
		
	}
}