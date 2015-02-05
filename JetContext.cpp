#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "JetContext.h"
#include "UniquePtr.h"

#include <stack>
#include <fstream>
#include <memory>
#undef Yield;
using namespace Jet;

Value Jet::gc(JetContext* context,Value* args, int numargs) 
{ 
	context->RunGC();
	return Value();
};

Value Jet::print(JetContext* context,Value* args, int numargs) 
{ 
	for (int i = 0; i < numargs; i++)
	{
		printf("%s", args[i].ToString().c_str());
	}
	printf("\n");
	return Value();
};

Value& JetContext::operator[](const std::string& id)
{
	auto iter = variables.find(id);
	if (iter == variables.end())
	{
		//add it
		variables[id] = variables.size();
		vars.push_back(Value());
		return vars[variables[id]];
	}
	else
	{
		return vars[(*iter).second];
	}
}

Value JetContext::Get(const std::string& name)
{
	auto iter = variables.find(name);
	if (iter == variables.end())
	{
		return Value();//return null
	}
	else
	{
		return vars[(*iter).second];
	}
}

void JetContext::Set(const std::string& name, const Value& value)
{
	auto iter = variables.find(name);
	if (iter == variables.end())
	{
		//add it
		variables[name] = variables.size();
		vars.push_back(value);
	}
	else
	{
		vars[(*iter).second] = value;
	}
}

Value JetContext::NewObject()
{
	auto v = new JetObject(this);
	v->refcount = 0;
	v->grey = v->mark = false;
	this->gc.objects.push_back(v);
	return Value(v);
}

ValueRef JetContext::NewPrototype(const char* Typename)
{
	auto v = new JetObject(this);
	v->refcount = 0;
	v->grey = v->mark = false;
	this->gc.objects.push_back(v);
	return v;
}

Value JetContext::NewArray()
{
	auto a = new JetArray;
	a->refcount = 0;
	a->grey = a->mark = false;
	a->ptr = new _JetArrayBacking;
	this->gc.arrays.push_back(a);
	return Value(a);
}

Value JetContext::NewUserdata(void* data, const Value& proto)
{
	if (proto.type != ValueType::Object)
		throw RuntimeException("NewUserdata: Prototype supplied was not of the type 'object'\n");

	auto ud = new GCVal<std::pair<void*, JetObject*>>(std::pair<void*, JetObject*>(data, proto._object));
	ud->grey = ud->mark = false;
	ud->refcount = 0;
	this->gc.userdata.push_back(ud);
	return Value(ud, proto._object);
}

Value JetContext::NewString(const char* string, bool copy)
{
	if (copy)
	{
		size_t len = strlen(string);
		auto temp = new char[len+1];
		memcpy(temp, string, len);
		temp[len] = 0;
		string = temp;
	}
	auto str = new GCVal<char*>((char*)string);
	str->grey = str->mark = false;
	str->refcount = 0;
	this->gc.strings.push_back(str);
	return Value(str);
}

JetContext::JetContext() : gc(this), stack(500000)
{
	this->labelposition = 0;
	this->fptr = 0;
	this->sptr = this->localstack;

	//add more functions and junk
	(*this)["print"] = print;
	(*this)["gc"] = ::gc;

	(*this)["setprototype"] = [](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid Call, Improper Arguments!");

		if (v->type == ValueType::Object && v[1].type == ValueType::Object)
		{
			Value val = v[0];
			val._object->prototype = v[1]._object;
			return val;
		}
		else
		{
			throw RuntimeException("Improper arguments!");
		}
	};

	(*this)["require"] = [](JetContext* context, Value* v, int args)
	{
		if (args != 1 || v->type != ValueType::String)
			throw RuntimeException("Invalid Call, Improper Arguments!");

		auto iter = context->require_cache.find(v->_string->ptr);
		if (iter == context->require_cache.end())
		{
			//load from file
			std::ifstream t(v->_string->ptr);
			if (t)
			{
				int length;
				t.seekg(0, std::ios::end);    // go to the end
				length = t.tellg();           // report location (this is the length)
				t.seekg(0, std::ios::beg);    // go back to the beginning
				UniquePtr<char[]> buffer(new char[length+1]);    // allocate memory for a buffer of appropriate dimension
				t.read(buffer, length);       // read the whole file into the buffer
				buffer[length] = 0;
				t.close();

				auto out = context->Compile(buffer, v->_string->ptr);
				auto fun = context->Assemble(out);
				context->require_cache[v->_string->ptr] = Value();
				auto obj = context->Call(&fun);
				context->require_cache[v->_string->ptr] = obj;
				return obj;
			}
			else
			{
				throw RuntimeException("Require could not find file: '" + (std::string)v->_string->ptr + "'");
			}
		}
		else
		{
			return Value(iter->second);
		}
	};

	(*this)["getprototype"] = [](JetContext* context, Value* v, int args)
	{
		if (args == 1 && (v->type == ValueType::Object || v->type == ValueType::Userdata))
			return Value(v->GetPrototype());
		else
			throw RuntimeException("getprototype expected an object or userdata value!");
	};


	this->file = new JetObject(this);
	file->prototype = 0;
	(*file)["read"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid number of arguments to read!");

		char* out = new char[((int)v->value)+1];//context->GCAllocate((v)->value);
		fread(out, 1, (int)(v)->value, v[1].GetUserdata<FILE>());
		out[(int)(v)->value] = 0;
		return context->NewString(out, false);
	});
	(*file)["write"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid number of arguments to write!");

		std::string str = v->ToString();
		fwrite(str.c_str(), 1, str.length(), v[1].GetUserdata<FILE>());
		return Value();
	});
	//this function is called when the value is garbage collected
	(*file)["_gc"] = Value([](JetContext* context, Value* v, int args)
	{
		fclose(v->GetUserdata<FILE>());
		return Value();
	});

	(*this)["fopen"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid number of arguments to fopen!");

		FILE* f = fopen(v->ToString().c_str(), v[1].ToString().c_str());
		return context->NewUserdata(f, context->file);
	});

	(*this)["fclose"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 1)
			throw RuntimeException("Invalid number of arguments to fclose!");
		fclose(v->GetUserdata<FILE>());
		return Value();
	});


	//setup the string and array tables
	this->string = new JetObject(this);
	this->string->prototype = 0;
	(*this->string)["append"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 2 && v[0].type == ValueType::String && v[1].type == ValueType::String)
		{
			size_t len = v[0].length + v[1].length + 1;
			char* text = new char[len];
			memcpy(text, v[1]._string, v[1].length);
			memcpy(text+v[1].length, v[0]._string, v[0].length);
			text[len-1] = 0;
			return context->NewString(text, false);
		}
		else
			throw RuntimeException("bad append call!");
	});
	(*this->string)["length"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 1)
			return Value((double)v->length);
		else
			throw RuntimeException("bad length call!");
	});

	this->Array = new JetObject(this);
	this->Array->prototype = 0;
	(*this->Array)["add"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 2)
		{
			v[1]._array->ptr->push_back(*v);
			//(*(v+1)->_array->ptr)[(v+1)->_array->ptr->size()] = *(v);
		}
		else
			throw RuntimeException("Invalid add call!!");
		return Value();
	});
	(*this->Array)["size"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 1)
			return Value((int)v->_array->ptr->size());
		else
			throw RuntimeException("Invalid size call!!");
	});
	(*this->Array)["resize"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 2)
			v[1]._array->ptr->resize(v->value);
		else
			throw RuntimeException("Invalid size call!!");
		return Value();
	});

	(*this->Array)["getIterator"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 1)
		{
			struct iter
			{
				std::vector<Value>* container;
				std::vector<Value>::iterator iterator;
			};
			iter* it = new iter;
			it->container = v->_array->ptr;
			it->iterator = v->_array->ptr->begin();
			if (it->iterator == v->_array->ptr->end())
			{
				delete it;
				return Value();
			}
			return Value(context->NewUserdata(it, context->arrayiter));
		}
		throw RuntimeException("Bad call to getIterator");
	});
	this->object = new JetObject(this);
	this->object->prototype = 0;
	(*this->object)["size"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 1)
			return Value((int)v->_object->size());
		else
			throw RuntimeException("Invalid size call!!");
	});

	(*this->object)["getIterator"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 1)
		{
			struct iter2
			{
				JetObject* container;
				JetObject::Iterator iterator;
			};
			iter2* it = new iter2;
			it->container = v->_object;
			it->iterator = v->_object->begin();
			if (it->iterator == v->_object->end())
			{
				delete it;
				return Value();
			}
			return (context->NewUserdata(it, context->objectiter));
		}
		throw RuntimeException("Bad call to getIterator");
	});
	this->objectiter = new JetObject(this);
	this->objectiter->prototype = 0;
	(*this->objectiter)["next"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			JetObject* container;
			JetObject::Iterator iterator;
		};
		auto iterator = v->GetUserdata<iter2>();
		if (iterator->iterator != iterator->container->end())
		{
			return Value((*iterator->iterator++).second);
			//++iterator->iterator;//->operator++();
		}
		//just return null
	});
	(*this->objectiter)["current"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			JetObject* container;
			JetObject::Iterator iterator;
		};
		//still kinda wierd
		auto iterator = v->GetUserdata<iter2>();
		if (iterator->iterator != iterator->container->end())
		{
			return Value((*iterator->iterator).second);
		}
		//just return null
	});
	(*this->objectiter)["advance"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			JetObject* container;
			JetObject::Iterator iterator;
		};
		auto iterator = v->GetUserdata<iter2>();
		if (++iterator->iterator != iterator->container->end())
			return Value(1);
		else
			return Value(0);
		//just return null
	});
	(*this->objectiter)["_gc"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			JetObject* container;
			JetObject::Iterator iterator;
		};
		delete v->GetUserdata<iter2>();
		return Value();
	});
	this->arrayiter = new JetObject(this);
	this->arrayiter->prototype = 0;
	(*this->arrayiter)["next"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			_JetArrayBacking* container;
			_JetArrayIterator iterator;
		};
		auto iterator = v->GetUserdata<iter>();
		if (iterator->iterator != iterator->container->end())
		{
			return Value((*iterator->iterator++));
			//++iterator->iterator;//->operator++();
		}
		//just return null
	});
	(*this->arrayiter)["current"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			_JetArrayBacking* container;
			_JetArrayIterator iterator;
		};
		//still kinda wierd
		auto iterator = v->GetUserdata<iter>();
		if (iterator->iterator != iterator->container->end())
			return Value((*iterator->iterator));
	});
	(*this->arrayiter)["advance"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			_JetArrayBacking* container;
			_JetArrayIterator iterator;
		};
		auto iterator = v->GetUserdata<iter>();
		if (++iterator->iterator != iterator->container->end())
			return Value(1);
		else
			return Value(0);
	});
	(*this->arrayiter)["_gc"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			_JetArrayBacking* container;
			_JetArrayIterator iterator;
		};
		delete v->GetUserdata<iter>();
		return Value();
	});
};

JetContext::~JetContext()
{
	this->gc.Cleanup();

	for (auto ii: this->ins)
		if (ii.instruction != InstructionType::LdStr)
			delete[] ii.string;

	for (auto ii: this->functions)
		delete ii.second;

	for (auto ii: this->entrypoints)
		delete ii;

	delete this->string;
	delete this->Array;
	delete this->file;
	delete this->object;
	delete this->arrayiter;
	delete this->objectiter;
}

#ifndef _WIN32
typedef signed long long INT64;
#endif
INT64 rate;
std::vector<IntermediateInstruction> JetContext::Compile(const char* code, const char* filename)
{
#ifdef JET_TIME_EXECUTION
	INT64 start, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif

	Lexer lexer = Lexer(code, filename);
	Parser parser = Parser(&lexer);

	//printf("In: %s\n\nResult:\n", code);
	BlockExpression* result = parser.parseAll();
	//result->print();
	//printf("\n\n");

	std::vector<IntermediateInstruction> out = compiler.Compile(result, filename);

	delete result;

#ifdef JET_TIME_EXECUTION
	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to compile\n\n", dt);
#endif

	return std::move(out);
}


class StackProfile
{
	char* name;
	INT64 start;
public:
	StackProfile(char* name)
	{
		this->name = name;
#ifdef JET_TIME_EXECUTION
#ifdef _WIN32
		//QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
		QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif
#endif
	};

	~StackProfile()
	{
#ifdef JET_TIME_EXECUTION
#ifdef _WIN32
		INT64 end;
		QueryPerformanceCounter( (LARGE_INTEGER *)&end );

		char o[100];
		INT64 diff = end - start;
		float dt = ((float)diff)/((float)rate);
		printf("%s took %f seconds\n", name, dt);
#endif
#endif
	}
};

void JetContext::RunGC()
{
	this->gc.Run();
}

Value JetContext::Execute(int iptr)
{
#ifdef JET_TIME_EXECUTION
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif
	//frame and stack pointer reset
	fptr += 1;//this needs to disappear

	unsigned int startcallstack = this->callstack.size();
	unsigned int startstack = this->stack.size();
	auto startlocalstack = this->sptr;

	callstack.Push(std::pair<unsigned int, Closure*>(123456789, 0));//bad value to get it to return;

	//printf("Execute: Stack Ptr At: %d\n", sptr - localstack);

	try
	{
		int max = ins.size();
		while(iptr < max && iptr >= 0)
		{
			Instruction in = ins[iptr];
			switch(in.instruction)
			{
			case InstructionType::Add:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two+one);
					break;
				}
			case InstructionType::Sub:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two-one);
					break;
				}
			case InstructionType::Mul:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two*one);
					break;
				}
			case InstructionType::Div:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two/one);
					break;
				}
			case InstructionType::Modulus:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two%one);
					break;
				}
			case InstructionType::BAnd:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two&one);
					break;
				}
			case InstructionType::BOr:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two|one);
					break;
				}
			case InstructionType::Xor:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two^one);
					break;
				}
			case InstructionType::BNot:
				{
					Value one = stack.Pop();
					stack.Push(~one);
					break;
				}
			case InstructionType::LeftShift:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two<<one);
					break;
				}
			case InstructionType::RightShift:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();
					stack.Push(two>>one);
					break;
				}
			case InstructionType::Incr:
				{
					Value one = stack.Pop();

					stack.Push(one+Value(1));
					break;
				}
			case InstructionType::Decr:
				{
					Value one = stack.Pop();

					stack.Push(one-Value(1));
					break;
				}
			case InstructionType::Negate:
				{
					Value one = stack.Pop();
					stack.Push(-one);
					break;
				}
			case InstructionType::Eq:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();

					if (one == two)
						stack.Push(Value(1));
					else
						stack.Push(Value(0));

					break;
				}
			case InstructionType::NotEq:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();

					if (one == two)
						stack.Push(Value(0));
					else
						stack.Push(Value(1));

					break;
				}
			case InstructionType::Lt:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();

					if ((double)one > (double)two)
						stack.Push(Value(1));
					else
						stack.Push(Value(0));

					break;
				}
			case InstructionType::Gt:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();

					if ((double)one < (double)two)
						stack.Push(Value(1));
					else
						stack.Push(Value(0));

					break;
				}
			case InstructionType::GtE:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();

					if ((double)one <= (double)two)
						stack.Push(Value(1));
					else
						stack.Push(Value(0));

					break;
				}
			case InstructionType::LtE:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();

					if ((double)one >= (double)two)
						stack.Push(Value(1));
					else
						stack.Push(Value(0));

					break;
				}
			case InstructionType::LdNull:
				{
					stack.Push(Value());
					break;
				}
			case InstructionType::LdNum:
				{
					stack.Push(in.value2);
					break;
				}
			case InstructionType::LdStr:
				{
					//fix this
					//fixme all these strings shouldnt be garbage collected
					stack.Push(Value(in.strlit));
					break;
				}
			case InstructionType::Jump:
				{
					iptr = in.value-1;
					break;
				}
			case InstructionType::JumpTrue:
				{
					auto temp = stack.Pop();
					switch (temp.type)
					{
					case ValueType::Number:
						if (temp.value != 0.0)
							iptr = in.value-1;
						break;
					case ValueType::Null:
						break;
					default:
						iptr = in.value-1;
					}
					//if ((int)temp)
					//	iptr = in.value-1;
					break;
				}
			case InstructionType::JumpFalse:
				{
					auto temp = stack.Pop();
					switch (temp.type)
					{
					case ValueType::Number:
						if (temp.value == 0.0)
							iptr = in.value-1;
						break;
					case ValueType::Null:
						iptr = in.value-1;
						break;
					}
					//if (!(int)temp)
					//iptr = in.value-1;
					break;
				}
			case InstructionType::Load:
				{
					stack.Push(vars[in.value]);

					break;
				}
			case InstructionType::Store:
				{
					auto temp = stack.Pop();
					//store me
					vars[in.value] = temp;
					break;
				}
			case InstructionType::LLoad:
				{
					//printf("Load at: Stack Ptr: %d\n", sptr - localstack + in.value);
					stack.Push(sptr[in.value]);
					break;
				}
			case InstructionType::LStore:
				{
					sptr[in.value] = stack.Pop();
					//printf("Store at: Stack Ptr: %d\n", sptr - localstack + in.value);
					break;
				}
			case InstructionType::CLoad:
				{
					auto frame = curframe;
					int index = in.value2;
					while ( index++ < 0)
						frame = frame->prev;

					if (frame->closed)
						stack.Push(frame->cupvals[in.value]);
					else
						stack.Push(*frame->upvals[in.value]);
					break;
				}
			case InstructionType::CStore:
				{
					auto frame = curframe;
					int index = in.value2;
					while ( index++ < 0)
						frame = frame->prev;

					//write barrier
					if (frame->grey == true)
					{
						frame->mark = false;
						gc.greys.Push(frame);
					}

					if (frame->closed)
						frame->cupvals[in.value] = stack.Pop();
					else
						*frame->upvals[in.value] = stack.Pop();
					break;
				}
			case InstructionType::LoadFunction:
				{
					//construct a new closure with the right number of upvalues
					//from the Func* object
					Closure* closure = new Closure;
					closure->grey = closure->mark = false;
					closure->prev = curframe;
					closure->refcount = 0;
					closure->generator = 0;
					closure->numupvals = in.func->upvals;
					closure->closed = false;
					if (in.func->upvals)
						closure->upvals = new Value*[in.func->upvals];
					closure->prototype = in.func;
					gc.closures.push_back(closure);
					stack.Push(Value(closure));

					if (gc.allocationCounter++%GC_INTERVAL == 0)
						this->RunGC();

					break;
				}
			case InstructionType::CInit:
				{
					//allocate and add new upvalue
					curframe->upvals[(unsigned int)in.value2] = &sptr[in.value];

					break;
				}
			case InstructionType::Close:
				{
					auto cur = curframe;
					if (cur && cur->numupvals && cur->closed == false)
					{
						auto tmp = new Value[cur->numupvals];
						for (int i = 0; i < cur->numupvals; i++)
							tmp[i] = *cur->upvals[i];

						delete[] cur->upvals;
						cur->closed = true;
						cur->cupvals = tmp;
					}

					break;
				}
			case InstructionType::Call:
				{
					//allocate capture area here
					if (vars[in.value].type == ValueType::Function)
					{
						//store iptr on call stack
						if (fptr > JET_MAX_CALLDEPTH)
							throw RuntimeException("Exceeded Max Call Depth!");

						if (vars[in.value]._function->prototype->generator)
						{
							//create generator and return it
							Closure* closure = new Closure;
							closure->grey = closure->mark = false;
							closure->prev = curframe->prev;
							closure->numupvals = closure->numupvals;
							closure->closed = false;
							closure->refcount = 0;
							closure->generator = new Generator(this, vars[in.value]._function, in.value2);
							if (closure->numupvals)
								closure->upvals = new Value*[closure->numupvals];
							closure->prototype = vars[in.value]._function->prototype;
							this->gc.closures.push_back(closure);

							this->stack.Push(Value(closure));
							break;
						}

						fptr++;

						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));

						this->sptr += curframe->prototype->locals;

						//clean out the new stack for the gc
						for (int i = 0; i < vars[in.value]._function->prototype->locals; i++)
						{
							sptr[i] = Value();
						}

						//need to reduce use of this if possible
						if ((sptr - localstack) >= JET_STACK_SIZE)
							throw RuntimeException("Stack Overflow!");

						curframe = vars[in.value]._function;

						if (curframe->closed)
						{
							//allocate new local frame here
							Closure* closure = new Closure;
							closure->grey = closure->mark = false;
							closure->prev = curframe->prev;
							closure->refcount = 0;
							closure->generator = 0;
							closure->numupvals = curframe->numupvals;
							closure->closed = false;
							if (closure->numupvals)
								closure->upvals = new Value*[closure->numupvals];
							closure->prototype = curframe->prototype;
							this->gc.closures.push_back(closure);

							curframe = closure;

							if (gc.allocationCounter++%GC_INTERVAL == 0)
								this->RunGC();
						}
						//printf("Call: Stack Ptr At: %d\n", sptr - localstack);

						Function* func = curframe->prototype;
						//set all the locals
						if ((unsigned int)in.value2 <= func->args)
						{
							for (int i = func->args-1; i >= 0; i--)
							{
								if (i < (int)in.value2)
									sptr[i] = stack.Pop();
								else
									sptr[i] = Value();
							}
						}
						else if (func->vararg)
						{
							sptr[func->locals-1] = this->NewArray();
							auto arr = sptr[func->locals-1]._array->ptr;
							arr->resize(in.value2 - func->args);
							for (int i = (int)in.value2-1; i >= 0; i--)
							{
								if (i < func->args)
									sptr[i] = stack.Pop();
								else
									(*arr)[i] = stack.Pop();
							}
						}
						else
						{
							for (int i = (int)in.value2-1; i >= 0; i--)
							{
								if (i < func->args)
									sptr[i] = stack.Pop();//frames[fptr].locals[i] = stack.Pop();
								else
									stack.Pop();
							}
						}

						//go to function
						iptr = func->ptr-1;
					}
					else if (vars[in.value].type == ValueType::NativeFunction)
					{
						unsigned int args = (unsigned int)in.value2;
						Value* tmp = &stack.mem[stack.size()-args];

						//ok fix this to be cleaner and resolve stack printing
						//should just push a value to indicate that we are in a native function call
						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));
						callstack.Push(std::pair<unsigned int, Closure*>(123456789, curframe));

						Value ret = (*vars[in.value].func)(this,tmp,args);
						stack.QuickPop(args);//pop off args

						callstack.QuickPop(2);
						stack.Push(ret);
					}
					else
					{
						//find the variable name from the in.value which is the index into the variable array
						std::string var;
						for (auto ii: variables)
						{
							if (ii.second == in.value)
							{
								var = ii.first;
								break;
							}
						}
						throw RuntimeException("Cannot call non function '" + var + " of type " + vars[(int)in.value].Type() + "'!!!");
					}

					break;
				}
			case InstructionType::ECall:
				{
					//allocate capture area here
					Value fun = stack.Pop();
					if (fun.type == ValueType::Function)
					{
						if (fptr > JET_MAX_CALLDEPTH)
							throw RuntimeException("Exceeded Max Call Depth!");

						if (fun._function->prototype->generator)
						{
							//create generator and return it
							Closure* closure = new Closure;
							closure->grey = closure->mark = false;
							closure->prev = curframe->prev;
							closure->numupvals = closure->numupvals;
							closure->closed = false;
							closure->refcount = 0;
							closure->generator = new Generator(this, curframe, in.value);
							if (closure->numupvals)
								closure->upvals = new Value*[closure->numupvals];
							closure->prototype = fun._function->prototype;
							this->gc.closures.push_back(closure);

							this->stack.Push(Value(closure));
							break;
						}

						fptr++;

						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));//callstack.Push(iptr);

						sptr += curframe->prototype->locals;

						//clean out the new stack for the gc
						for (int i = 0; i < fun._function->prototype->locals; i++)
						{
							sptr[i] = Value();
						}

						if ((sptr - localstack) >= JET_STACK_SIZE)
							throw RuntimeException("Stack Overflow!");

						curframe = fun._function;

						if (curframe->closed)
						{
							//allocate new local frame here
							Closure* closure = new Closure;
							closure->grey = closure->mark = false;
							closure->prev = curframe->prev;
							closure->numupvals = closure->numupvals;
							closure->closed = false;
							closure->refcount = 0;
							closure->generator = 0;
							if (closure->numupvals)
								closure->upvals = new Value*[closure->numupvals];
							closure->prototype = curframe->prototype;
							this->gc.closures.push_back(closure);

							curframe = closure;

							if (gc.allocationCounter++%GC_INTERVAL == 0)
								this->RunGC();
						}
						//printf("ECall: Stack Ptr At: %d\n", sptr - localstack);

						Function* func = curframe->prototype;
						//set all the locals
						if (in.value <= func->args)
						{
							for (int i = func->args-1; i >= 0; i--)
							{
								if (i < in.value)
									sptr[i] = stack.Pop();
								else
									sptr[i] = Value();
							}
						}
						else if (func->vararg)
						{
							sptr[func->locals-1] = this->NewArray();
							auto arr = sptr[func->locals-1]._array->ptr;
							arr->resize(in.value2 - func->args);
							for (int i = (int)in.value2-1; i >= 0; i--)
							{
								if (i < func->args)
									sptr[i] = stack.Pop();
								else
									(*arr)[i] = stack.Pop();
							}
						}
						else
						{
							for (int i = in.value-1; i >= 0; i--)
							{
								if (i < func->args)
									sptr[i] = stack.Pop();
								else
									stack.Pop();
							}
						}

						//go to function
						iptr = fun._function->prototype->ptr-1;
					}
					else if (fun.type == ValueType::NativeFunction)
					{
						unsigned int args = (unsigned int)in.value;
						Value* tmp = &stack.mem[stack.size()-args];

						//ok fix this to be cleaner and resolve stack printing
						//should just push a value to indicate that we are in a native function call
						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));
						callstack.Push(std::pair<unsigned int, Closure*>(123456789, curframe));

						Value ret = (*fun.func)(this,tmp,args);
						stack.QuickPop(args);

						callstack.QuickPop(2);
						stack.Push(ret);
					}
					else
					{
						throw RuntimeException("Cannot call non function type " + std::string(fun.Type()) + "!!!");
					}
					break;
				}
			case InstructionType::Return:
				{
					auto oframe = callstack.Pop();
					iptr = oframe.first;
					if (this->curframe && this->curframe->generator)
						this->curframe->generator->Kill();

					if (oframe.first != 123456789)
					{
#ifdef _DEBUG
						//this makes sure that the gc doesnt overrun its boundaries
						for (int i = 0; i < oframe.second->prototype->locals; i++)
						{
							//need to mark stack with garbage values for error checking
							sptr[i].type = ValueType::Object;
							sptr[i]._object = (JetObject*)0xcdcdcdcd;
						}
#endif
						sptr -= oframe.second->prototype->locals;//curframe->prototype->locals;
					}
					//printf("Return: Stack Ptr At: %d\n", sptr - localstack);
					curframe = oframe.second;

					fptr--;
					break;
				}
			case InstructionType::Yield:
				{
					if (this->curframe->generator)
						this->curframe->generator->Yield(this, iptr);
					else
						throw RuntimeException("Cannot Yield from outside a generator");
					
					auto oframe = callstack.Pop();
					iptr = oframe.first;
					curframe = oframe.second;
					sptr -= oframe.second->prototype->locals;
					fptr--;

					break;
				}
			case InstructionType::Resume:
				{
					//resume last item placed on stack
					Value v = this->stack.Pop();
					if (v.type != ValueType::Function || v._function->generator == 0)
						throw RuntimeException("Cannot resume a non generator!");

					fptr++;

					callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));//callstack.Push(iptr);

					sptr += curframe->prototype->locals;

					curframe = v._function;

					iptr = v._function->generator->Resume(this)-1;

					break;
				}
			case InstructionType::Dup:
				{
					stack.Push(stack.Peek());
					break;
				}
			case InstructionType::Pop:
				{
					stack.Pop();
					break;
				}
			case InstructionType::StoreAt:
				{
					if (in.string)
					{
						Value loc = stack.Pop();
						Value val = stack.Pop();	

						if (loc.type == ValueType::Object)
							(*loc._object)[in.string] = val;
						else
							throw RuntimeException("Could not index a non array/object value!");

						if (loc._object->mark)
						{
							//reset to grey and push back for reprocessing
							//printf("write barrier triggered!\n");
							loc._object->mark = false;
							gc.greys.Push(loc);//push to grey stack
						}
						//write barrier
					}
					else
					{
						Value index = stack.Pop();
						Value loc = stack.Pop();
						Value val = stack.Pop();	

						if (loc.type == ValueType::Array)
						{
							//gc bug apparant in benchmark.txt.txt fixme
							if ((int)index >= loc._array->ptr->size() || (int)index < 0)
								throw RuntimeException("Array index out of range!");
							(*loc._array->ptr)[(int)index] = val;

							//write barrier
							if (loc._array->mark)
							{
								//reset to grey and push back for reprocessing
								//printf("write barrier triggered!\n");
								loc._array->mark = false;
								gc.greys.Push(loc);//push to grey stack
							}
						}
						else if (loc.type == ValueType::Object)
						{
							(*loc._object)[index] = val;

							//write barrier
							if (loc._object->mark)
							{
								//reset to grey and push back for reprocessing
								//printf("write barrier triggered!\n");
								loc._object->mark = false;
								gc.greys.Push(loc);//push to grey stack
							}
						}
						else
							throw RuntimeException("Could not index a non array/object value!");
						//write barrier
					}
					break;
				}
			case InstructionType::LoadAt:
				{
					if (in.string)
					{
						Value loc = stack.Pop();
						if (loc.type == ValueType::Object)
						{
							auto n = loc._object->findNode(in.string);
							if (n)
							{
								stack.Push(n->second);
							}
							else
							{
								auto obj = loc._object->prototype;
								while (obj)
								{
									n = obj->findNode(in.string);
									if (n)
									{
										stack.Push(n->second);
										break;
									}
									obj = obj->prototype;
								}
							}
						}
						else if (loc.type == ValueType::String)
							stack.Push((*this->string)[in.string]);
						else if (loc.type == ValueType::Array)
							stack.Push((*this->Array)[in.string]);
						else if (loc.type == ValueType::Userdata)
							stack.Push((*loc._userdata->ptr.second)[in.string]);
						else
							throw RuntimeException("Could not index a non array/object value!");
					}
					else
					{
						Value index = stack.Pop();
						Value loc = stack.Pop();

						if (loc.type == ValueType::Array)
						{
							if ((int)index >= loc._array->ptr->size())
								throw RuntimeException("Array index out of range!");
							stack.Push((*loc._array->ptr)[(int)index]);
						}
						else if (loc.type == ValueType::Object)
							stack.Push((*loc._object)[index]);
						else
							throw RuntimeException("Could not index a non array/object value!");
					}
					break;
				}
			case InstructionType::NewArray:
				{
					auto arr = new GCVal<std::vector<Value>*>(new std::vector<Value>(in.value));
					arr->grey = arr->mark = false;
					arr->refcount = 0;
					this->gc.arrays.push_back(arr);
					for (int i = in.value-1; i >= 0; i--)
						(*arr->ptr)[i] = stack.Pop();
					stack.Push(Value(arr));

					if (gc.allocationCounter++%GC_INTERVAL == 0)
						this->RunGC();

					break;
				}
			case InstructionType::NewObject:
				{
					auto obj = new JetObject(this);
					obj->grey = obj->mark = false;
					obj->refcount = 0;
					this->gc.objects.push_back(obj);
					for (int i = in.value-1; i >= 0; i--)
					{
						auto value = stack.Pop();
						auto key = stack.Pop();
						(*obj)[key] = value;
					}
					stack.Push(Value(obj));

					if (gc.allocationCounter++%GC_INTERVAL == 0)
						this->RunGC();

					break;
				}
			default:
				{
					throw RuntimeException("Unimplemented Instruction!");
				}
			}

			iptr++;
		}
	}
	catch(RuntimeException e)
	{
		if (e.processed == false)
		{
			printf("RuntimeException: %s\nCallstack:\n", e.reason.c_str());

			//generate call stack
			this->StackTrace(iptr);

			printf("\nLocals:\n");
			if (curframe)
			{
				for (int i = 0; i < curframe->prototype->locals; i++)
				{
					Value v = this->sptr[i];
					if (v.type >= ValueType(0))
						printf("%d = %s\n", i, v.ToString().c_str());
				}
			}

			printf("\nVariables:\n");
			for (auto ii: variables)
			{
				if (vars[ii.second].type != ValueType::Null)
					printf("%s = %s\n", ii.first.c_str(), vars[ii.second].ToString().c_str());
			}
			e.processed = true;
		}

		//ok, need to properly roll back callstack
		//this stuff is lets you call Jet functions from something that called Jet

		//make sure I reset everything in the event of an error

		//reset fptr
		fptr -= this->callstack.size()-startcallstack;

		//clear the stacks
		this->callstack.QuickPop(this->callstack.size()-startcallstack);
		this->stack.QuickPop(this->stack.size()-startstack);

		//reset the local variable stack
		this->sptr = startlocalstack;

		//ok add the exception details to the exception as a string or something rather than just printing them
		//maybe add more details to the exception when rethrowing
		throw e;
	}
	catch(...)
	{
		printf("Caught Some Other Exception\n\nCallstack:\n");

		this->StackTrace(iptr);

		printf("\nVariables:\n");
		for (auto ii: variables)
		{
			printf("%s = %s\n", ii.first.c_str(), vars[ii.second].ToString().c_str());
		}

		//reset fptr
		fptr -= this->callstack.size()-startcallstack;

		//ok, need to properly roll back callstack
		this->callstack.QuickPop(this->callstack.size()-startcallstack);
		this->stack.QuickPop(this->stack.size()-startstack);

		//reset the local variable stack
		this->sptr = startlocalstack;
	}


#ifdef JET_TIME_EXECUTION
	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to execute\n\n", dt);
#endif

#ifdef _DEBUG
	//debug checks for stack and what not
	if (this->callstack.size() == 0)
	{
		if (this->sptr != this->localstack)
			throw RuntimeException("FATAL ERROR: Local stack did not properly reset");

		if (this->fptr != 0)
			throw RuntimeException("FATAL ERROR: Frame pointer did not properly reset");
	}

	//check for stack leaks
	if (this->stack.size() > startstack+1)
		throw RuntimeException("FATAL ERROR: Stack leak detected!");
#endif

	return stack.Pop();
}

void JetContext::GetCode(int ptr, std::string& ret, unsigned int& line)
{
	int imax = this->debuginfo.size()-1;
	int imin = 0;
	while (imax >= imin)
	{
		// calculate the midpoint for roughly equal partition
		int imid = (imin+imax)/2;//midpoint(imin, imax);
		if(this->debuginfo[imid].code == ptr)
		{
			// key found at index imid
			ret = this->debuginfo[imid].file;
			line = this->debuginfo[imid].line;
			return;// imid; 
		}
		// determine which subarray to search
		else if (this->debuginfo[imid].code < ptr)
			// change min index to search upper subarray
			imin = imid + 1;
		else         
			// change max index to search lower subarray
			imax = imid - 1;
	}
#undef min
	int index = std::min(imin, imax);
	if (index < 0)
		index = 0;

	if (this->debuginfo.size() == 0)//make sure we have debug info
	{
		line = 1;
		return;
	}

	ret = this->debuginfo[index].file;
	line = this->debuginfo[index].line;
}

void JetContext::StackTrace(int curiptr)
{
	auto tempcallstack = this->callstack.Copy();
	tempcallstack.Push(std::pair<unsigned int, Closure*>(curiptr,0));

	while(tempcallstack.size() > 0)
	{
		auto top = tempcallstack.Pop();
		int greatest = -1;
		std::string fun;
		for (auto ii: this->functions)
		{
			//ok, need to find which label pos I am most greatest than or equal to
			if (top.first >= ii.second->ptr && (int)ii.second->ptr > greatest)
			{
				fun = ii.first;
				greatest = ii.second->ptr;
			}
		}
		if (top.first == 123456789)
			printf("{Native}\n");
		else
		{
			std::string file;
			unsigned int line;
			this->GetCode(top.first, file, line);
			printf("%s() %s Line %d (Instruction %d)\n", fun.c_str(), file.c_str(), line, top.first-greatest);
		}
	}
}

Value JetContext::Assemble(const std::vector<IntermediateInstruction>& code)
{
#ifdef JET_TIME_EXECUTION
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif
	std::map<std::string, unsigned int> labels;

	for (auto inst: code)
	{
		switch (inst.type)
		{
		case InstructionType::Comment:
			{
				break;
			}
		case InstructionType::DebugLine:
			{
				//this should contain line/file info
				DebugInfo info;
				info.file = inst.string;
				info.line = inst.second;
				info.code = this->labelposition;
				this->debuginfo.push_back(info);
				//push something into the array at the instruction pointer
				delete[] inst.string;

				break;
			}
		case InstructionType::Function:
			{
				//do something with argument and local counts
				Function* func = new Function;
				func->args = inst.a;
				func->locals = inst.b;
				func->upvals = inst.c;
				func->ptr = labelposition;
				func->name = inst.string;
				func->generator = inst.d & 2 ? true : false;
				func->vararg = inst.d & 1? true : false;

				if (functions.find(inst.string) == functions.end())
					functions[inst.string] = func;
				else if (strcmp(inst.string, "{Entry Point}") == 0)
				{
					//have to do something with old entry point because it leaks
					entrypoints.push_back(functions[inst.string]);
					functions[inst.string] = func;
				}
				else
					throw RuntimeException("ERROR: Duplicate Function Label Name: %s\n" + std::string(inst.string));

				delete[] inst.string;
				break;
			}
		case InstructionType::Label:
			{
				if (labels.find(inst.string) == labels.end())
				{
					labels[inst.string] = this->labelposition;
					delete[] inst.string;
				}
				else
				{
					delete[] inst.string;
					throw RuntimeException("ERROR: Duplicate Label Name: %s\n" + std::string(inst.string));
				}
				break;
			}
		default:
			{
				this->labelposition++;
			}
		}
	}

	for (auto inst: code)
	{
		switch (inst.type)
		{
		case InstructionType::Comment:
		case InstructionType::Function:
		case InstructionType::Label:
		case InstructionType::DebugLine:
			{
				break;
			}
		default:
			{
				Instruction ins;
				ins.instruction = inst.type;
				ins.string = inst.string;
				ins.value = inst.first;
				ins.value2 = inst.second;

				switch (inst.type)
				{
				case InstructionType::Call:
				case InstructionType::Store:
				case InstructionType::Load:
					{
						if (variables.find(inst.string) == variables.end())
						{
							//add it
							variables[inst.string] = variables.size();
							vars.push_back(Value());
						}
						ins.value = variables[inst.string];
						break;
					}
				case InstructionType::LdStr:
					{
						Value str = this->NewString(inst.string, false);
						str.AddRef();
						ins.strlit = str._string;
						break;
					}
				case InstructionType::LoadFunction:
					{
						ins.func = functions[inst.string];
						break;
					}
				case InstructionType::Jump:
				case InstructionType::JumpFalse:
				case InstructionType::JumpTrue:
					{
						if (labels.find(inst.string) == labels.end())
							throw RuntimeException("Label '" + (std::string)inst.string + "' does not exist!");
						ins.value = labels[inst.string];
						break;
					}
				case InstructionType::ForEach:
					{
						if (labels.find(inst.string) == labels.end())
							throw RuntimeException("Label '" + (std::string)inst.string + "' does not exist!");
						ins.value = labels[inst.string];
						if (labels.find(inst.string2) == labels.end())
							throw RuntimeException("Label '" + (std::string)inst.string2 + "' does not exist!");
						ins.value2 = labels[inst.string2];

						delete[] inst.string;
						delete[] inst.string2;
						break;
					}
				}

				this->ins.push_back(ins);
			}
		}
	}

	if (labels.size() > 100000)
		throw CompilerException("test", 5, "problem with labels!");

	auto tmpframe = new Closure;
	tmpframe->grey = tmpframe->mark = false;
	tmpframe->prev = 0;
	tmpframe->closed = false;
	tmpframe->generator = 0;
	tmpframe->prototype = this->functions["{Entry Point}"];
	tmpframe->numupvals = tmpframe->prototype->upvals;
	if (tmpframe->numupvals)
		tmpframe->upvals = new Value*[tmpframe->numupvals];
	else
		tmpframe->upvals = 0;

	gc.closures.push_back(tmpframe);

#ifdef JET_TIME_EXECUTION
	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to assemble\n\n", dt);
#endif

	return tmpframe;
};


Value JetContext::Call(const Value* fun, Value* args, unsigned int numargs)
{
	if (fun->type != ValueType::NativeFunction && fun->type != ValueType::Function)
	{
		printf("ERROR: Variable is not a function\n");
		return Value();
	}
	else if (fun->type == ValueType::NativeFunction)
	{
		//call it
		int s = this->stack.size();
		(*fun->func)(this,args,numargs);
		if (s == this->stack.size())
			return Value();

		return this->stack.Pop();
	}
	unsigned int iptr = fun->_function->prototype->ptr;

	//clear stack values for the gc
	for (int i = 0; i < fun->_function->prototype->locals; i++)
		sptr[i] = Value();

	//push args onto stack
	for (unsigned int i = 0; i < numargs; i++)
		this->stack.Push(args[i]);

	auto func = fun->_function;
	if (numargs <= func->prototype->args)
	{
		for (int i = func->prototype->args-1; i >= 0; i--)
		{
			if (i < numargs)
				sptr[i] = stack.Pop();
			else
				sptr[i] = Value();
		}
	}
	else if (func->prototype->vararg)
	{
		sptr[func->prototype->locals-1] = this->NewArray();
		auto arr = sptr[func->prototype->locals-1]._array->ptr;
		arr->resize(numargs - func->prototype->args);
		for (int i = numargs-1; i >= 0; i--)
		{
			if (i < func->prototype->args)
				sptr[i] = stack.Pop();
			else
				(*arr)[i] = stack.Pop();
		}
	}
	else
	{
		for (int i = numargs-1; i >= 0; i--)
		{
			if (i < func->prototype->args)
				sptr[i] = stack.Pop();
			else
				stack.Pop();
		}
	}

	this->curframe = fun->_function;

	return this->Execute(iptr);
}

//executes a function in the VM context
Value JetContext::Call(const char* function, Value* args, unsigned int numargs)
{
	int iptr = 0;
	//printf("Calling: '%s'\n", function);
	if (variables.find(function) == variables.end())
	{
		printf("ERROR: No variable named: '%s' to call\n", function);
		return Value(0);
	}

	Value fun = vars[variables[function]];
	if (fun.type != ValueType::NativeFunction && fun.type != ValueType::Function)
	{
		printf("ERROR: Variable '%s' is not a function\n", function);
		return Value(0);
	}

	return this->Call(&fun, args, numargs);
};

std::string JetContext::Script(const std::string code, const std::string filename)
{
	try
	{
		//try and execute
		return this->Script(code.c_str(), filename.c_str()).ToString();
	}
	catch(CompilerException E)
	{
		printf("Exception found:\n");
		printf("%s (%d): %s\n", E.file.c_str(), E.line, E.ShowReason());
		return "";
	}
}

Value JetContext::Script(const char* code, const char* filename)//compiles, assembles and executes the script
{
	auto asmb = this->Compile(code, filename);
	Value fun = this->Assemble(asmb);

	return this->Call(&fun);
}

#ifdef EMSCRIPTEN
#include <emscripten/bind.h>
using namespace emscripten;
using namespace std;
EMSCRIPTEN_BINDINGS(Jet) {
	class_<Jet::JetContext>("JetContext")
		.constructor()
		.function<std::string, std::string, std::string>("Script", &Jet::JetContext::Script);
}
#endif