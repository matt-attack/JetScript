#ifndef _VALUE_HEADER
#define _VALUE_HEADER

#include <map>
#include <functional>
#include <string>

namespace Jet
{
	class JetContext;

	template<class t>
	struct GCVal
	{
		bool flag;
		t ptr;

		GCVal(t t)
		{
			ptr = t;
		}
	};

	struct Value;

	typedef GCVal<std::map<std::string, Value>*> _JetObject; 
	typedef GCVal<std::map<int, Value>*> _JetArray; 
	typedef void(*_JetNativeFunc)(JetContext*,Value*, int);///*std::function<void(JetContext*,Value*,int)>*/ _JetNativeFunc;

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
			//std::string string;
			struct string
			{
				unsigned int len;
				char* data;
			} string;
			_JetArray* _array;
			_JetObject* _obj;
			void* object;//used for userdata
			unsigned int ptr;//used for functions, points to code
			_JetNativeFunc func;//native func
		};

		Value()
		{
			this->type = ValueType::Null;
		};

		Value(const char* str)
		{
			if (str == 0)
				return;
			type = ValueType::String;

			string.len = strlen(str)+10;
 			char* news = new char[string.len];
			strcpy(news, str);
			string.data = news;
		}

		Value(_JetObject* obj)
		{
			this->type = ValueType::Object;
			this->_obj = obj;
		}

		Value(_JetArray* arr)
		{
			type = ValueType::Array;
			this->_array = arr;
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

		/*Value(std::function<void(JetContext*, Value*, int)>* ptr)
		{
			type = ValueType::NativeFunction;
			func = *a;
		}*/

		Value(void* userdata)
		{
			this->type = ValueType::Userdata;
			this->object = userdata;
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
				return this->string.data;
			case ValueType::Function:
				return "[Function "+::std::to_string(this->ptr)+"]"; 
			case ValueType::NativeFunction:
				return "[NativeFunction "+::std::to_string((unsigned int)this->func)+"]";
			case ValueType::Array:
				{
					std::string str = "[Array " + std::to_string((int)this->_array)+"]";//"[\n";
					for (auto ii: *this->_array->ptr)
					{
						str += "\t";
						str += ::std::to_string(ii.first);
						str += " = ";
						str += ii.second.ToString() + "\n";
					}
					str += "]";
					return str;
				}
			case ValueType::Object:
				{
					std::string str = "[Object " + std::to_string((int)this->_obj)+"]";
					for (auto ii: *this->_obj->ptr)
					{
						str += "\t";
						str += ii.first;
						str += " = ";
						str += ii.second.ToString() + "\n";
					}
					str += "}";
					return str;
				}
			case ValueType::Userdata:
				{
					return "[Userdata "+std::to_string((int)this->object)+"]";
				}
			default:
				return "";
			}
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
				return Value(value+other.value);
			case ValueType::String:
				{
					if (other.type == ValueType::String)
						return Value((std::string(other.string.data) + std::string(this->string.data)).c_str());
				}
			}

			return Value(0);
		};

		Value operator[] (int index)
		{
			if (this->type == ValueType::Array)
				return (*this->_array->ptr)[index];
			else if (this->type == ValueType::Object)
				return (*this->_obj->ptr)[std::to_string(index)];
			return Value(0);
		};

		Value operator-( const Value &other )
		{
			if (type == ValueType::Number)
				return Value(value-other.value);

			return Value(0);
		};

		Value operator*( const Value &other )
		{
			if (type == ValueType::Number)
				return Value(value*other.value);

			return Value(0);
		};

		Value operator/( const Value &other )
		{
			if (type == ValueType::Number)
				return Value(value/other.value);

			return Value(0);
		};

		Value operator%( const Value &other )
		{
			if (type == ValueType::Number)
				return Value((int)value%(int)other.value);

			return Value(0);
		};
	};
}

#endif