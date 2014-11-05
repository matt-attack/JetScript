#ifndef _VALUE_HEADER
#define _VALUE_HEADER

#include <map>
#include <functional>
#include <string>
#include <memory>
#include <vector>

#include "JetExceptions.h"

namespace Jet
{
	class JetContext;

	template<class t>
	struct GCVal
	{
		bool mark;
		bool grey;
		t ptr;

		GCVal() { }

		GCVal(t tt)
		{
			ptr = tt;
		}
	};

	struct Value;

	typedef GCVal<std::map<std::string, Value>*> _JetObject; 
	typedef GCVal<std::vector<Value>* > _JetArray;//std::map<int, Value>*> _JetArray; 
	typedef GCVal<std::pair<void*,_JetObject*> > _JetUserdata;
	typedef char _JetString;//GCVal<char*> _JetString;

	typedef void _JetFunction;
	typedef void(*_JetNativeFunc)(JetContext*,Value*, int);
	//typedef std::function<void(JetContext*,Value*,int)> _JetNativeFunc;

	enum class ValueType
	{
		//keep all garbage collectable types towards the end after Object
		//this is used for the GC being able to tell what it is quickly
		Null = 0,
		Number,
		NativeFunction,
		String,//add more
		Object,//todo
		Array,//todo
		Function,
		//Closure,//an allocated one
		Userdata,//kinda todo
	};

	static const char* ValueTypes[] = { "Null", "Number", "NativeFunction", "String" , "Object", "Array", "Function", "Userdata"};

	class JetContext;

	struct Function
	{
		unsigned int ptr;
		unsigned int args, locals, upvals;
		std::string name;
	};

	struct Closure
	{
		bool mark;
		bool grey;
		unsigned short numupvals;
		Value* upvals;
		Function* prototype;

		Closure* prev;
	};

	struct Value
	{
		ValueType type;
		union
		{
			double value;
			long long integer;//todo implement me
			//this is the new main struct
			struct
			{
				union
				{
					_JetString* _string;
					_JetObject* _object;
					_JetArray* _array;
					_JetUserdata* _userdata;
				};
				_JetObject* prototype;
			} _obj;

			struct
			{
				Closure* _function;
				//Function* _function;
				//unsigned int ptr;//used for functions, points to code
			};
			_JetNativeFunc func;//native func
		};

		Value()
		{
			this->type = ValueType::Null;
			//this->value = 0;
		};

		//plz dont delete my string
		Value(const char* str)
		{
			if (str == 0)
				return;
			type = ValueType::String;

			_obj._string = (char*)str;
		}

		Value(_JetObject* obj)
		{
			this->type = ValueType::Object;
			this->_obj._object = obj;
			this->_obj.prototype = 0;
		}

		Value(_JetArray* arr)
		{
			type = ValueType::Array;
			this->_obj._array = arr;
		}

		Value(double val)
		{
			type = ValueType::Number;
			value = val;
		}

		Value(int val)
		{
			type = ValueType::Number;
			value = val;
		}

		Value(_JetNativeFunc a)
		{
			type = ValueType::NativeFunction;
			func = a;
		}

		Value(Closure* func)
		{
			type = ValueType::Function;
			_function = func;
		}

		explicit Value(_JetUserdata* userdata, _JetObject* prototype)
		{
			this->type = ValueType::Userdata;
			this->_obj._userdata = userdata;
			this->_obj.prototype = prototype;
		}

		void SetPrototype(_JetObject* obj)
		{
			_obj.prototype = obj;
		}

		std::string ToString()
		{
			switch(this->type)
			{
			case ValueType::Null:
				return "Null";
			case ValueType::Number:
				return ::std::to_string(this->value);
			case ValueType::String:
				return this->_obj._string;
			case ValueType::Function:
				return "[Function "+::std::to_string(this->_function->prototype->ptr)+"]"; 
			case ValueType::NativeFunction:
				return "[NativeFunction "+::std::to_string((unsigned int)this->func)+"]";
			case ValueType::Array:
				{
					std::string str = "[Array " + std::to_string((int)this->_obj._array)+"]";//"[\n";
					/*for (auto ii: *this->_array->ptr)
					{
					str += "\t";
					str += ::std::to_string(ii.first);
					str += " = ";
					str += ii.second.ToString() + "\n";
					}
					str += "]";*/
					return str;
				}
			case ValueType::Object:
				{
					std::string str = "[Object " + std::to_string((int)this->_obj._object)+"]";
					/*for (auto ii: *this->_obj->ptr)
					{
					str += "\t";
					str += ii.first;
					str += " = ";
					str += ii.second.ToString() + "\n";
					}
					str += "}";*/
					return str;
				}
			case ValueType::Userdata:
				{
					return "[Userdata "+std::to_string((int)this->_obj._userdata)+"]";
				}
			default:
				return "";
			}
		}

		template<class T>
		inline T*& GetUserdata()
		{
			return (T*&)this->_obj._userdata->ptr.first;
		}

		const char* Type()
		{
			return ValueTypes[(int)this->type];
		}

		operator int()
		{
			if (type == ValueType::Number)
				return (int)value;

			return 0;
		};

		operator double()
		{
			if (type == ValueType::Number)
				return value;
			return 0;
		}

		_JetObject* GetPrototype()
		{
			//add defaults for string and array
			switch (type)
			{
			case ValueType::Array:
				return 0;
			case ValueType::Object:
				return this->_obj.prototype;
			case ValueType::String:
				return 0;
			case ValueType::Userdata:
				return this->_obj.prototype;
			default:
				return 0;
			}
		}

		/*template<typename First, typename... Types>
		Value operator() (First f, Types... args)
		{
		print("hi");
		//this->CallHelper(args...);
		}*/
		template<typename First, typename Second>
		Value operator() (JetContext* context, First first, First second)
		{

		}

		template<typename First>
		Value operator() (JetContext* context, First first)
		{

		}

		Value operator() (JetContext* context)
		{

		}

		/*template<typename First>
		Value operator() (First f)
		{
		this->CallHelper(f);
		}*/

		/*template<typename First, typename... Types>
		void CallHelper(First t)
		{
		//actually call
		//this->CallHelper<
		}

		template<typename First>
		void CallHelper(First t)
		{
		//actually call
		}*/


		Value operator[] (int index)
		{
			if (this->type == ValueType::Array)
				return (*this->_obj._array->ptr)[index];
			else if (this->type == ValueType::Object)
				return (*this->_obj._object->ptr)[std::to_string(index)];
			return Value(0);
		};

		Value operator[] (Value key)
		{
			switch (type)
			{
			case ValueType::Array:
				{
					return (*this->_obj._array->ptr)[(int)key.value];
				}
			case ValueType::Object:
				{
					return (*this->_obj._object->ptr)[key.ToString()];
				}
			default:
				throw RuntimeException("Cannot index type " + (std::string)ValueTypes[(int)this->type]);
			}
		}

		Value operator+( const Value &other )
		{
			switch(this->type)
			{
			case ValueType::Number:
				if (other.type == ValueType::Number)
					return Value(value+other.value);
			case ValueType::Object:
				{
					//throw JetRuntimeException("Cannot Add A String");
					//if (other.type == ValueType::String)
					//return Value((std::string(other.string.data) + std::string(this->string.data)).c_str());
					//else
					//return Value((other.ToString() + std::string(this->string.data)).c_str());
				}
			}

			throw RuntimeException("Cannot add two non-numeric types! " + (std::string)ValueTypes[(int)other.type] + " and " + (std::string)ValueTypes[(int)this->type]);
		};

		Value operator-( const Value &other )
		{
			if (type == ValueType::Number && other.type == ValueType::Number)
				return Value(value-other.value);
			else if (type == ValueType::Object)
			{
				//do metamethod
			}

			throw RuntimeException("Cannot subtract two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
		};

		Value operator*( const Value &other )
		{
			if (type == ValueType::Number && other.type == ValueType::Number)
				return Value(value*other.value);
			else if (type == ValueType::Object)
			{
				//do metamethod
			}

			throw RuntimeException("Cannot multiply two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
		};

		Value operator/( const Value &other )
		{
			if (type == ValueType::Number && other.type == ValueType::Number)
				return Value(value/other.value);
			else if (type == ValueType::Object)
			{
				//do metamethod
			}

			throw RuntimeException("Cannot divide two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
		};

		Value operator%( const Value &other )
		{
			if (type == ValueType::Number && other.type == ValueType::Number)
				return Value((int)value%(int)other.value);
			else if (type == ValueType::Object)
			{
				//do metamethod
			}

			throw RuntimeException("Cannot modulus two non-numeric types! " + (std::string)ValueTypes[(int)this->type] + " and " + (std::string)ValueTypes[(int)other.type]);
		};
	};
}

#endif