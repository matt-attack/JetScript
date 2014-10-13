#ifndef _VALUE_HEADER
#define _VALUE_HEADER

#include <map>
#include <functional>
#include <string>
#include <memory>

#include "JetExceptions.h"

namespace Jet
{
	class JetContext;

	template<class t>
	struct GCVal
	{
		bool flag;
		t ptr;

		GCVal() { }

		GCVal(t t)
		{
			ptr = t;
		}
	};

	struct Value;

	typedef GCVal<std::map<std::string, Value>*> _JetObject; 
	typedef GCVal<std::map<int, Value>*> _JetArray; 
	typedef GCVal<std::pair<void*,_JetObject*>> _JetUserdata;
	typedef GCVal<char*> _JetString;

	typedef void(*_JetNativeFunc)(JetContext*,Value*, int);
	typedef void(*_JetNativeInstanceFunc)(JetContext*,Value*,Value*,int);
	//typedef std::function<void(JetContext*,Value*,int)> _JetNativeFunc;

	enum class ValueType
	{
		Number = 0,
		String,//add more
		Object,//todo
		Array,//todo
		Function,
		NativeFunction,
		Userdata,//kinda todo
		Null,
	};

	class JetContext;

	struct Value
	{
		ValueType type;
		union
		{
			double value;
			//this is the new main struct
			struct object
			{
				union
				{
					char* _string;
					//_JetString* _string;
					_JetObject* _object;
					_JetArray* _array;
					_JetUserdata* _userdata;
				};
				_JetObject* prototype;
			} _obj;

			unsigned int ptr;//used for functions, points to code
			_JetNativeFunc func;//native func
			_JetNativeInstanceFunc instancefunc;
		};

		Value()
		{
			this->type = ValueType::Null;
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

		Value(unsigned int pos, bool func)
		{
			type = ValueType::Function;
			ptr = pos;
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
				return "[Function "+::std::to_string(this->ptr)+"]"; 
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

		Value operator+( const Value &other )
		{
			switch(this->type)
			{
			case ValueType::Number:
				if (other.type == ValueType::Number)
					return Value(value+other.value);
				else
					throw JetRuntimeException("Cannot add two non-numeric types!");
			case ValueType::String:
				{
					//throw JetRuntimeException("Cannot Add A String");
					//if (other.type == ValueType::String)
					//return Value((std::string(other.string.data) + std::string(this->string.data)).c_str());
					//else
					//return Value((other.ToString() + std::string(this->string.data)).c_str());
				}
			}

			throw JetRuntimeException("Cannot add two non-numeric types!");
			
			return Value(0);
		};

		Value operator[] (int index)
		{
			if (this->type == ValueType::Array)
				return (*this->_obj._array->ptr)[index];
			else if (this->type == ValueType::Object)
				return (*this->_obj._object->ptr)[std::to_string(index)];
			return Value(0);
		};

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
				//default:
				//throw JetRuntimeException("Cannot index type");
			}
		}

		Value operator-( const Value &other )
		{
			if (type == ValueType::Number && other.type == ValueType::Number)
				return Value(value-other.value);

			throw JetRuntimeException("Cannot subtract two non-numeric types!");

			return Value(0);
		};

		Value operator*( const Value &other )
		{
			if (type == ValueType::Number && other.type == ValueType::Number)
				return Value(value*other.value);

			throw JetRuntimeException("Cannot multiply two non-numeric types!");

			return Value(0);
		};

		Value operator/( const Value &other )
		{
			if (type == ValueType::Number && other.type == ValueType::Number)
				return Value(value/other.value);

			throw JetRuntimeException("Cannot divide two non-numeric types!");
			return Value(0);
		};

		Value operator%( const Value &other )
		{
			if (type == ValueType::Number && other.type == ValueType::Number)
				return Value((int)value%(int)other.value);

			throw JetRuntimeException("Cannot modulus two non-numeric types!");

			return Value(0);
		};
	};
}

#endif