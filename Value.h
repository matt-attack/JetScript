#ifndef _VALUE_HEADER
#define _VALUE_HEADER

#include <map>
#include <functional>
#include <string>

namespace Jet
{
	enum class ValueType
	{
		Number = 0,
		String,//add more
		Object,//todo
		Array,//todo
		Function,
		NativeFunction,
		Userdata,//kinda todo
	};

	class JetContext;

	struct Value
	{
		ValueType type;
		union
		{
			double value;
			struct string
			{
				unsigned int len;
				char* data;
			} string;
			std::map<int,Value>* _array;
			std::map<std::string, Value>* _obj;
			void* object;//used for userdata
			unsigned int ptr;//used for functions, points to code
			std::function<void(JetContext*,Value*,int)>* func;//native func
		};

		Value()
		{
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

		Value(std::map<std::string, Value>* obj)
		{
			this->type = ValueType::Object;
			this->_obj = obj;
		}

		Value(std::map<int, Value>* arr)
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

		Value(std::function<void(JetContext*,Value*,int)>* a)
		{
			type = ValueType::NativeFunction;
			func = a;
		}

		Value(unsigned int pos, bool func)
		{
			type = ValueType::Function;
			ptr = pos;
		}

		Value(void* userdata)
		{
			this->type = ValueType::Userdata;
			this->object = userdata;
		}

		std::string ToString()
		{
			switch(this->type)
			{
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
					std::string str = "[Array " + std::to_string((int)this->_array)+"] [\n";//"[\n";
					for (auto ii: *this->_array)
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
					std::string str = "[Object " + std::to_string((int)this->_obj)+"] {\n";
					for (auto ii: *this->_obj)
					{
						str += "\t";
						str += ii.first;
						str += " = ";
						str += ii.second.ToString() + "\n";
					}
					str += "}";
					return str;
				}
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
				//case ValueType::Function:
				//return "[Function "+std::to_string(this->ptr)+"]"; 
				//case ValueType::NativeFunction:
				//return "[NativeFunction "+std::to_string((unsigned int)this->func)+"]";
			}
			//if (type == ValueType::Number)
			//return Value(value+other.value);
			//else if (type == ValueType::String)
			//if (other.type == ValueType::String)
			//return Value((std::string(other.string.data) + std::string(this->string.data)).c_str());

			return Value(0);
		};

		Value operator[] (int index)
		{
			if (this->type == ValueType::Array)
				return (*this->_array)[index];
			else if (this->type == ValueType::Object)
				return (*this->_obj)[std::to_string(index)];
			//cant really do anything in here
		};

		//Value operator[] (std::string index)
		//{
		//todo me for objects
		//};

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