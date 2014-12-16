#include "Value.h"

using namespace Jet;

inline size_t stringhash(const char* p)
{
	size_t tot = *p;
	while (*(p++))
	{
		tot *= *p;
		tot -= 3*(*p);
	}
	return tot;
}

std::size_t HashFunction::operator ()(const Value &v) const
{
	switch(v.type)
	{
	case ValueType::Null:
		return 0;
	case ValueType::Array:
	case ValueType::NativeFunction:
	case ValueType::Object:
		return (size_t)v._array;
	case ValueType::Number:
		return v.value;
	case ValueType::String:
		return stringhash(v._string);
	case ValueType::Function:
		return (size_t)v._function;
	}
};