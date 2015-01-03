#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "JetContext.h"

#include <stack>

using namespace Jet;

void Jet::gc(JetContext* context,Value* args, int numargs) 
{ 
	context->RunGC();
};

void Jet::print(JetContext* context,Value* args, int numargs) 
{ 
	for (int i = 0; i < numargs; i++)
	{
		printf("%s", args[i].ToString().c_str());
	}
	printf("\n");
};

Value JetContext::NewObject()
{
	auto v = new _JetObject;
	v->grey = v->mark = false;
	v->ptr = new _JetObjectBacking;
	this->objects.push_back(v);
	return Value(v);
}

_JetObject* JetContext::NewPrototype(const std::map<std::string, Value>& items, const char* Typename)
{ 
	auto v = new _JetObject;
	v->grey = v->mark = false;
	auto table = new _JetObjectBacking;
	v->ptr = table;
	for (auto ii: items)
	{
		char* str = new char[ii.first.size()+1];
		ii.first.copy(str, ii.first.length());
		str[ii.first.size()] = 0;
		(*table)[str] = ii.second;
	}
	return v;
}

Value JetContext::NewArray()
{
	auto a = new _JetArray;
	a->grey = a->mark = false;
	a->ptr = new _JetArrayBacking;
	this->arrays.push_back(a);
	return Value(a);
}

Value JetContext::NewUserdata(void* data, _JetObject* proto)
{
	auto ud = new GCVal<std::pair<void*, _JetObject*>>(std::pair<void*, _JetObject*>(data, proto));
	ud->grey = ud->mark = true;
	this->userdata.push_back(ud);
	return Value(ud, proto);
}

Value JetContext::NewString(char* string)
{
	this->strings.push_back(new GCVal<char*>(string));
	return Value(string);
}

JetContext::JetContext()
{
	this->allocationCounter = 1;//messes up if these start at 0
	this->collectionCounter = 1;

	this->labelposition = 0;
	this->fptr = -1;
	stack = VMStack<Value>(500000);
	//add more functions and junk
	(*this)["print"] = print;
	(*this)["gc"] = gc;
	(*this)["setprototype"] = [](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid Call, Improper Arguments!");

		if (v->type == ValueType::Object && (v+1)->type == ValueType::Object)
		{
			Value val = *v;
			val.prototype = (v+1)->_object;
			context->Return(val);
		}
		else
		{
			throw RuntimeException("Improper arguments!");
		}
	};

	(*this)["setprototype"] = [](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid Call, Improper Arguments!");

		if (v->type == ValueType::Object && (v+1)->type == ValueType::Object)
		{
			Value val = *v;
			val.prototype = (v+1)->_object;
			context->Return(val);
		}
		else
		{
			throw RuntimeException("Improper arguments!");
		}
	};

	(*this)["getprototype"] = [](JetContext* context, Value* v, int args)
	{
		if (args == 1 && (v->type == ValueType::Object || v->type == ValueType::Userdata))
		{
			context->Return(v->GetPrototype());
		}
		else
			throw RuntimeException("getprototype expected an object or userdata value!");
	};


	this->file.ptr = new _JetObjectBacking;//std::map<std::string, Value>;
	(*file.ptr)["read"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid number of arguments to read!");

		char* out = new char[((int)v->value)+1];//context->GCAllocate((v)->value);
		fread(out, 1, (int)(v)->value, (v+1)->GetUserdata<FILE>());
		out[(int)(v)->value] = 0;
		context->Return(context->NewString(out));
	});
	(*file.ptr)["write"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid number of arguments to write!");

		std::string str = v->ToString();
		fwrite(str.c_str(), 1, str.length(), (v+1)->GetUserdata<FILE>());
	});
	//this function is called when the value is garbage collected
	(*file.ptr)["_gc"] = Value([](JetContext* context, Value* v, int args)
	{
		fclose(v->GetUserdata<FILE>());
	});


	(*this)["fopen"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid number of arguments to fopen!");

		FILE* f = fopen(v->ToString().c_str(), (*(v+1)).ToString().c_str());
		context->Return(context->NewUserdata(f, &context->file));
	});

	(*this)["fclose"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 1)
			throw RuntimeException("Invalid number of arguments to fclose!");
		fclose(v->GetUserdata<FILE>());
	});


	//setup the string and array tables
	this->string.ptr = new _JetObjectBacking;//std::map<std::string, Value>;
	(*this->string.ptr)["append"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 2)
			context->Return(Value("newstr"));
		else
			throw RuntimeException("bad append call!");
	});
	(*this->string.ptr)["length"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 1)
			context->Return(Value((double)strlen(v->_string)));
		else
			throw RuntimeException("bad length call!");
	});

	this->Array.ptr = new _JetObjectBacking;//std::map<std::string, Value>;
	(*this->Array.ptr)["add"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 2)
		{
			(v+1)->_array->ptr->push_back(*v);
			//(*(v+1)->_array->ptr)[(v+1)->_array->ptr->size()] = *(v);
		}
		else
			throw RuntimeException("Invalid add call!!");
	});
	(*this->Array.ptr)["size"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 1)
			context->Return((int)v->_array->ptr->size());
		else
			throw RuntimeException("Invalid size call!!");
	});
	(*this->Array.ptr)["resize"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 2)
			(v+1)->_array->ptr->resize((*v).value);
		else
			throw RuntimeException("Invalid size call!!");
	});

	(*this->Array.ptr)["getIterator"] = Value([](JetContext* context, Value* v, int args)
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
			context->Return(context->NewUserdata(it, &context->arrayiter));
			return;
		}
		throw RuntimeException("Bad call to getIterator");
	});
	this->object.ptr = new _JetObjectBacking;//std::map<std::string, Value>;
	(*this->object.ptr)["size"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 1)
			context->Return((int)v->_object->ptr->size());
		else
			throw RuntimeException("Invalid size call!!");
	});

	(*this->object.ptr)["getIterator"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 1)
		{
			struct iter2
			{
				_JetObjectBacking* container;
				_JetObjectIterator iterator;
			};
			iter2* it = new iter2;
			it->container = v->_object->ptr;
			it->iterator = v->_object->ptr->begin();
			context->Return(context->NewUserdata(it, &context->objectiter));
			return;
		}
		throw RuntimeException("Bad call to getIterator");
	});
	this->objectiter.ptr = new _JetObjectBacking;//std::map<std::string, Value>;
	(*this->objectiter.ptr)["next"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			_JetObjectBacking* container;
			_JetObjectIterator iterator;
		};
		auto iterator = v->GetUserdata<iter2>();
		if (iterator->iterator != iterator->container->end())
		{
			context->Return((*iterator->iterator).second);
			++iterator->iterator;//->operator++();
		}
		//just return null
	});
	(*this->objectiter.ptr)["current"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			_JetObjectBacking* container;
			_JetObjectIterator iterator;
		};
		//still kinda wierd
		auto iterator = v->GetUserdata<iter2>();
		if (iterator->iterator != iterator->container->end())
		{
			context->Return((*iterator->iterator).second);
		}
		//just return null
	});
	(*this->objectiter.ptr)["advance"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			_JetObjectBacking* container;
			_JetObjectIterator iterator;
		};
		auto iterator = v->GetUserdata<iter2>();
		if (++iterator->iterator != iterator->container->end())
			context->Return(1);
		else
			context->Return(0);
		//just return null
	});
	(*this->objectiter.ptr)["_gc"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			_JetObjectBacking* container;
			_JetObjectIterator iterator;
		};
		delete v->GetUserdata<iter2>();
	});
	this->arrayiter.ptr = new _JetObjectBacking;//std::map<std::string, Value>;
	(*this->arrayiter.ptr)["next"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			_JetArrayBacking* container;
			_JetArrayIterator iterator;
		};
		auto iterator = v->GetUserdata<iter>();
		if (iterator->iterator != iterator->container->end())
		{
			context->Return((*iterator->iterator));
			++iterator->iterator;//->operator++();
		}
		//just return null
	});
	(*this->arrayiter.ptr)["current"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			_JetArrayBacking* container;
			_JetArrayIterator iterator;
		};
		//still kinda wierd
		auto iterator = v->GetUserdata<iter>();
		if (iterator->iterator != iterator->container->end())
		{
			context->Return((*iterator->iterator));
		}
		//just return null
	});
	(*this->arrayiter.ptr)["advance"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			_JetArrayBacking* container;
			_JetArrayIterator iterator;
		};
		auto iterator = v->GetUserdata<iter>();
		if (++iterator->iterator != iterator->container->end())
			context->Return(1);
		else
			context->Return(0);
		//just return null
	});
	(*this->arrayiter.ptr)["_gc"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			_JetArrayBacking* container;
			_JetArrayIterator iterator;
		};
		delete v->GetUserdata<iter>();
	});
};

JetContext::~JetContext()
{
	for (auto ii: this->arrays)
	{
		delete ii->ptr;
		delete ii;
	}

	for (auto ii: this->userdata)
	{
		if (ii->ptr.second)
		{
			Value ud = Value(ii, ii->ptr.second);
			Value _gc = (*ii->ptr.second->ptr)["_gc"];
			if (_gc.type == ValueType::NativeFunction)
				_gc.func(this, &ud, 1);
			else if (_gc.type != ValueType::Null)
				throw RuntimeException("Not Implemented!");
		}
		delete ii;
	}

	for (auto ii: this->objects)
	{
		delete ii->ptr;
		delete ii;
	}

	for (auto ii: this->strings)
	{
		delete ii->ptr;
		delete ii;
	}

	for (auto ii: this->ins)
		delete[] ii.string;

	for (auto ii: this->functions)
		delete ii.second;

	for (auto ii: this->entrypoints)
		delete ii;

	for (auto ii: this->closures)
	{
		if (ii->numupvals)
			delete[] ii->upvals;
		delete ii;
	}

	delete this->string.ptr;
	delete this->Array.ptr;
	delete this->file.ptr;
	delete this->object.ptr;
	delete this->arrayiter.ptr;
	delete this->objectiter.ptr;
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

	std::vector<IntermediateInstruction> out = compiler.Compile(result);

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
	//printf("Running GC: %d Greys, %d Globals, %d Stack\n%d Closures, %d Arrays, %d Objects, %d Userdata\n", this->greys.size(), this->vars.size(), 0, this->closures.size(), this->arrays.size(), this->objects.size(), this->userdata.size());
#ifdef JET_TIME_EXECUTION
	INT64 start, end;
	//QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif
	//add more write barriers to detect when objects are removed and what not
	//if flag is marked, then black
	//if no flag and grey bit, then grey
	//if no flag or grey bit, then white

	//push all reachable items onto grey stack
	//this means globals
	{
		StackProfile profile("Mark Globals as Grey");
		for (int i = 0; i < this->vars.size(); i++)
		{
			if (vars[i].type > ValueType::NativeFunction)
			{
				if (/*vars[i]._object->mark == false && */vars[i]._object->grey == false)
				{
					this->vars[i]._object->grey = true;
					this->greys.Push(this->vars[i]);
				}
			}
		}
	}

	//get me working with strings
	bool nextIncremental = ((this->collectionCounter+1)%GC_STEPS)!=0;
	/*if (this->collectionCounter % GC_STEPS == 0)
	printf("Full Collection!\n");
	else
	printf("Incremental Collection!\n");*/

	if (this->stack.size() > 0)
	{
		StackProfile prof("Make Stack Grey");
		for (int i = 0; i < this->stack.size(); i++)
		{
			if (stack.mem[i].type > ValueType::NativeFunction)
			{
				if (stack.mem[i]._object->grey == false)
				{
					stack.mem[i]._object->grey = true;
					this->greys.Push(stack.mem[i]);
				}
			}
		}
	}


	//this is really part of the sweep section
	if (this->fptr >= 0)
	{
		StackProfile prof("Traverse/Mark Stack");
		//printf("GC run at runtime!\n");
		//traverse all local vars
		if (curframe->grey == false)
		{
			curframe->grey = true;
			this->greys.Push(Value(curframe));
		}

		int sp = 0;
		for (int i = 0; i < this->callstack.size(); i++)
		{
			auto closure = this->callstack[i].second;
			if (closure->grey == false)
			{
				closure->grey = true;
				this->greys.Push(Value(closure));
			}
			int max = sp+closure->prototype->locals;
			for (; sp < max; sp++)
			{
				if (this->localstack[sp].type > ValueType::NativeFunction)
				{
					if (this->localstack[sp]._object->grey == false)
					{
						this->localstack[sp]._object->grey = true;
						this->greys.Push(this->localstack[sp]);
					}
				}
			}
		}

		//mark curframe locals
		int max = sp+curframe->prototype->locals;
		for (; sp < max; sp++)
		{
			if (this->localstack[sp].type > ValueType::NativeFunction)
			{
				if (this->localstack[sp]._object->grey == false)
				{
					this->localstack[sp]._object->grey = true;
					this->greys.Push(this->localstack[sp]);
				}
			}
		}
	}


	{
		StackProfile prof("Traverse Greys");
		while(this->greys.size() > 0)
		{
			//traverse the object
			auto obj = this->greys.Pop();
			switch (obj.type)
			{
			case ValueType::Object:
				{
					if (obj.prototype && obj.prototype->grey == false)
					{
						obj.prototype->grey = true;
						obj.prototype->mark = true;

						for (auto ii: *obj.prototype->ptr)
						{
							if (ii.second.type > ValueType::NativeFunction)
							{
								ii.second._object->grey = true;
								greys.Push(ii.second);
							}
						}
					}

					obj._object->mark = true;
					for (auto ii: *obj._object->ptr)
					{
						if (ii.second.type > ValueType::NativeFunction && ii.second._object->grey == false)
						{
							ii.second._object->grey = true;
							greys.Push(ii.second);
						}
					}
					break;
				}
			case ValueType::Array:
				{
					obj._array->mark = true;

					for (auto ii: *obj._array->ptr)
					{
						if (ii.type > ValueType::NativeFunction && ii._object->grey == false)
						{
							ii._object->grey = true;
							greys.Push(ii);
						}
					}

					break;
				}
			case ValueType::String:
				{
					break;
				}
			case ValueType::Function:
				{
					obj._function->mark = true;
					//printf("Function Marked\n");
					if (obj._function->prev && obj._function->prev->grey == false)
					{
						obj._function->prev->grey = true;
						greys.Push(Value(obj._function->prev));
					}

					if (obj._function->closed)
					{
						for (int i = 0; i < obj._function->numupvals; i++)
						{
							if (obj._function->cupvals[i].type > ValueType::NativeFunction)
							{
								if (obj._function->cupvals[i]._object->grey == false)
								{
									obj._function->cupvals[i]._object->grey = true;
									greys.Push(obj._function->cupvals[i]);
								}
							}
						}
					}
					break;
				}
			case ValueType::Userdata:
				{
					obj._userdata->mark = true;

					if (obj.prototype)
					{
						obj.prototype->mark = true;

						for (auto ii: *obj.prototype->ptr)
							greys.Push(ii.second);
					}
					break;
				}
			}
		}
	}


	/* SWEEPING SECTION */


	//this must all be done when sweeping!!!!!!!

	//process stack variables, stack vars are ALWAYS grey

	//finally sweep through
	//sweep and free all whites and make all blacks white
	//iterate through all gc values

	//mark stack variables and globals
	{
		StackProfile prof("Sweep");
		auto olist = std::move(this->objects);
		this->objects.clear();
		for (auto ii: olist)
		{
			if (ii->mark)
			{
				if (nextIncremental == false)
				{
					ii->mark = false;
					ii->grey = false;
				}
				this->objects.push_back(ii);
			}
			else
			{
				delete ii->ptr;
				delete ii;
			}
		}

		auto alist = std::move(this->arrays);
		this->arrays.clear();
		for (auto ii: alist)
		{
			if (ii->mark)
			{
				if (nextIncremental == false)
				{
					ii->mark = false;
					ii->grey = false;
				}
				this->arrays.push_back(ii);
			}
			else
			{
				delete ii->ptr;
				delete ii;
			}
		}

		auto clist = std::move(this->closures);
		this->closures.clear();
		for (auto ii: clist)
		{
			if (ii->mark)
			{
				if (nextIncremental == false)
				{
					ii->mark = false;
					ii->grey = false;
				}
				this->closures.push_back(ii);
			}
			else
			{
				if (ii->numupvals)
					delete[] ii->upvals;
				delete ii;
			}
		}

		auto ulist = std::move(this->userdata);
		this->userdata.clear();
		for (auto ii: ulist)
		{
			if (ii->mark)
			{
				if (nextIncremental == false)
				{
					ii->mark = false;
					ii->grey = false;
				}
				this->userdata.push_back(ii);
			}
			else
			{
				if (ii->ptr.second)
				{
					Value ud = Value(ii, ii->ptr.second);
					Value _gc = (*ii->ptr.second->ptr)["_gc"];
					if (_gc.type == ValueType::NativeFunction)
						_gc.func(this, &ud, 1);
					else if (_gc.type != ValueType::Null)
						throw RuntimeException("Not Implemented!");
				}
				delete ii;
			}
		}
	}

	//Obviously, this approach doesn't work for a non-copying GC. But the main insights behind a generational GC can be abstracted:

	//Minor collections only take care of newly allocated objects.
	//Major collections deal with all objects, but are run much less often.

	//The basic idea is to modify the sweep phase: 
	//1. free the (unreachable) white objects, but don't flip the color of black objects before a minor collection. 
	//2. The mark phase of the following minor collection then only traverses newly allocated blocks and objects written to (marked gray). 
	//3. All other objects are assumed to be still reachable during a minor GC and are neither traversed, nor swept, nor are their marks changed (kept black). A regular sweep phase is used if a major collection is to follow.

	this->collectionCounter++;//used to determine collection mode
#ifdef JET_TIME_EXECUTION
	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to collect garbage\n\n", dt);
#endif
	//this->StackTrace(curframe->prototype->ptr);
	//printf("GC Complete: %d Greys, %d Globals, %d Stack\n%d Closures, %d Arrays, %d Objects, %d Userdata\n", this->greys.size(), this->vars.size(), 0, this->closures.size(), this->arrays.size(), this->objects.size(), this->userdata.size());
}

Value JetContext::Execute(int iptr)
{
#ifdef JET_TIME_EXECUTION
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif

	//frame and stack pointer reset
	fptr = 0;
	sptr = this->localstack;

	callstack.Push(std::pair<unsigned int, Closure*>(123456789, curframe));//bad value to get it to return;
	unsigned int startcallstack = this->callstack.size();
	unsigned int startstack = this->stack.size();

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
					stack.Push(one+two);
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
					stack.Push(one*two);
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
					stack.Push(-one.value);
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
					stack.Push(in.string);
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
						this->greys.Push(frame);
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
					closure->numupvals = in.func->upvals;
					closure->closed = false;
					if (in.func->upvals)
						closure->upvals = new Value*[in.func->upvals];
					closure->prototype = in.func;
					this->closures.push_back(closure);
					stack.Push(Value(closure));

					break;
				}
			case InstructionType::CInit:
				{
					//whelp
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

						fptr++;

						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));//callstack.Push(iptr);

						this->sptr += curframe->prototype->locals;

						if ((sptr - localstack) >= JET_STACK_SIZE)
							throw RuntimeException("Stack Overflow!");

						curframe = vars[in.value]._function;

						if (curframe->closed)
						{
							//allocate new local frame here
							Closure* closure = new Closure;
							closure->grey = closure->mark = false;
							closure->prev = curframe->prev;
							closure->numupvals = curframe->numupvals;//in.func->upvals;
							closure->closed = false;
							if (closure->numupvals)
								closure->upvals = new Value*[closure->numupvals];
							closure->prototype = curframe->prototype;//in.func;
							this->closures.push_back(closure);

							curframe = closure;
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
						stack.QuickPop(args);//pop off args

						//ok fix this to be cleaner and resolve stack printing
						//should just push a value to indicate that we are in a native function call
						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));//callstack.Push(iptr);
						callstack.Push(std::pair<unsigned int, Closure*>(123456789, curframe));//callstack.Push(123456789);
						//to return something, push it to the stack
						int s = stack.size();
						(*vars[in.value].func)(this,tmp,args);

						callstack.QuickPop(2);
						if (stack.size() == s)//we didnt return anything
							stack.Push(Value());//return null
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

						fptr++;

						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));//callstack.Push(iptr);

						sptr += curframe->prototype->locals;

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
							if (closure->numupvals)
								closure->upvals = new Value*[closure->numupvals];
							closure->prototype = curframe->prototype;
							this->closures.push_back(closure);

							curframe = closure;
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
									sptr[i] = stack.Pop();//frames[fptr].locals[i] = stack.Pop();
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
						stack.QuickPop(args);//pop off args

						//ok fix this to be cleaner and resolve stack printing
						//should just push a value to indicate that we are in a native function call
						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));
						//callstack.Push(iptr);
						callstack.Push(std::pair<unsigned int, Closure*>(123456789, curframe));
						//callstack.Push(123456789);
						//to return something, push it to the stack
						int s = stack.size();
						(*fun.func)(this,tmp,args);

						callstack.QuickPop(2);
						if (stack.size() == s)//we didnt return anything
							stack.Push(Value());//return null
					}
					else
					{
						throw RuntimeException("Cannot call non function type " + std::string(fun.Type()) + "!!!");
					}
					break;
				}
			case InstructionType::Return:
				{
					auto oframe = callstack.Pop();//iptr = callstack.Pop();
					iptr = oframe.first;
					sptr -= oframe.second->prototype->locals;//curframe->prototype->locals;
					//printf("Return: Stack Ptr At: %d\n", sptr - localstack);
					curframe = oframe.second;

					fptr--;
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
							(*loc._object->ptr)[in.string] = val;
						else
							throw RuntimeException("Could not index a non array/object value!");

						if (loc._object->mark)
						{
							//reset to grey and push back for reprocessing
							//printf("write barrier triggered!\n");
							loc._object->mark = false;
							this->greys.Push(loc);//push to grey stack
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
								this->greys.Push(loc);//push to grey stack
							}
						}
						else if (loc.type == ValueType::Object)
						{
							(*loc._object->ptr)[index] = val;

							//write barrier
							if (loc._object->mark)
							{
								//reset to grey and push back for reprocessing
								//printf("write barrier triggered!\n");
								loc._object->mark = false;
								this->greys.Push(loc);//push to grey stack
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
						//add metamethods
						if (loc.type == ValueType::Object)
						{
							Value v;
							auto ii = loc._object->ptr->find(in.string);
							if (ii == loc._object->ptr->end())
							{
								if (loc.prototype)
								{
									ii = loc.prototype->ptr->find(in.string);
									if (ii == loc.prototype->ptr->end())
									{
										ii = this->object.ptr->find(in.string);
										if (ii != this->object.ptr->end())
											v = ii->second;
									}
									else
										v = ii->second;
								}
								else
								{
									ii = this->object.ptr->find(in.string);
									if (ii != this->object.ptr->end())
										v = ii->second;
								}
								stack.Push(v);
							}
							else
								stack.Push(ii->second);
							//Value v = (*loc._object->ptr)[in.string];

							//check meta table
							//if (v.type == ValueType::Null && loc.prototype)
							//v = (*loc.prototype->ptr)[in.string];
							//if (v.type == ValueType::Null)
							//v = (*this->object.ptr)[in.string];
							//stack.Push(v);
						}
						else if (loc.type == ValueType::String)
							stack.Push((*this->string.ptr)[in.string]);
						else if (loc.type == ValueType::Array)
							stack.Push((*this->Array.ptr)[in.string]);
						else if (loc.type == ValueType::Userdata)
							stack.Push((*loc.prototype->ptr)[in.string]);
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
							stack.Push((*loc._object->ptr)[index]);
						else
							throw RuntimeException("Could not index a non array/object value!");
					}
					break;
				}
			case InstructionType::NewArray:
				{
					auto arr = new GCVal<std::vector<Value>*>(new std::vector<Value>(in.value));//new std::map<int, Value>;
					arr->grey = arr->mark = false;
					this->arrays.push_back(arr);
					for (int i = in.value-1; i >= 0; i--)
						(*arr->ptr)[i] = stack.Pop();
					stack.Push(Value(arr));

					if (allocationCounter++%GC_INTERVAL == 0)
						this->RunGC();

					break;
				}
			case InstructionType::NewObject:
				{
					auto obj = new _JetObject(new _JetObjectBacking);
					obj->grey = obj->mark = false;
					this->objects.push_back(obj);
					for (int i = in.value-1; i >= 0; i--)
						(*obj->ptr)[stack.Pop()] = stack.Pop();
					stack.Push(Value(obj));

					if (allocationCounter++%GC_INTERVAL == 0)
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

		//ok, need to properly roll back callstack
		//this stuff is lets you call Jet functions from something that called Jet
		/*this->callstack.QuickPop(this->callstack.size()-startcallstack);

		if (this->callstack.size() == 1 && this->callstack.Peek() == 123456789)
		this->callstack.Pop();*/

		//make sure I reset everything in the event of an error

		//reset fptr
		fptr = -1;

		//clear the stacks
		this->callstack.QuickPop(this->callstack.size()-startcallstack);
		this->stack.QuickPop(this->stack.size()-startstack);

		//this->stack.QuickPop(this->stack.size());
		//this->callstack.QuickPop(this->callstack.size());

		//should rethrow here or pass
		//to a handler or something

		//maybe add more details to the exception then rethrow
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

		//ok, need to properly roll back callstack
		this->callstack.QuickPop(this->callstack.size()-startcallstack);
		this->stack.QuickPop(this->stack.size()-startstack);

		if (this->callstack.size() == 1 && this->callstack.Peek().first == 123456789)
			this->callstack.Pop();
	}


#ifdef JET_TIME_EXECUTION
	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to execute\n\n", dt);
#endif

	this->fptr = -1;//set to invalid to indicate to the GC that we arent executing if it gets ran

	if (stack.size() == 0)
		return Value();

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
				//clear label array for each function
				//labels.clear();

				//do something with argument and local counts
				Function* func = new Function;
				func->args = inst.a;
				func->locals = inst.b;
				func->upvals = inst.c;
				func->ptr = labelposition;
				func->name = inst.string;
				func->vararg = inst.d ? true : false;

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
				if (this->labels.find(inst.string) == labels.end())
				{
					this->labels[inst.string] = this->labelposition;
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
				}

				this->ins.push_back(ins);
			}
		}
	}

#ifdef JET_TIME_EXECUTION
	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to assemble\n\n", dt);
#endif

	auto tmpframe = new Closure;
	tmpframe->prev = 0;
	//tmpframe->next = 0;
	tmpframe->closed = false;
	tmpframe->prototype = this->functions["{Entry Point}"];
	tmpframe->numupvals = tmpframe->prototype->upvals;
	if (tmpframe->numupvals)
		tmpframe->upvals = new Value*[tmpframe->numupvals];
	else
		tmpframe->upvals = 0;

	this->curframe = tmpframe;

	this->closures.push_back(tmpframe);

	Value temp = this->Execute(tmpframe->prototype->ptr);//run the static code
	if (this->callstack.size() > 0)
		this->callstack.Pop();

	return temp;
};


Value JetContext::Call(Value* fun, Value* args, unsigned int numargs)
{
	if (fun->type != ValueType::NativeFunction && fun->type != ValueType::Function)
	{
		throw 7;
	}
	else if (fun->type == ValueType::NativeFunction)
	{
		//call it
		(*fun->func)(this,args,numargs);
		return this->stack.Pop();//Value(0);
	}
	unsigned int iptr = fun->_function->prototype->ptr;

	//push args onto stack
	for (unsigned int i = 0; i < numargs; i++)
	{
		this->stack.Push(args[i]);
	}

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
				sptr[i] = stack.Pop();//frames[fptr].locals[i] = stack.Pop();
			else
				stack.Pop();
		}
	}

	this->curframe = fun->_function;

	Value temp = this->Execute(iptr);
	if (callstack.size() > 0)
		callstack.Pop();
	return temp;
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
	else if (fun.type == ValueType::NativeFunction)
	{
		//call it
		(*fun.func)(this,args,numargs);
		return Value(0);
	}
	iptr = fun._function->prototype->ptr;

	//push args onto stack
	for (unsigned int i = 0; i < numargs; i++)
	{
		this->stack.Push(args[i]);
	}

	auto func = fun._function;
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
				sptr[i] = stack.Pop();//frames[fptr].locals[i] = stack.Pop();
			else
				stack.Pop();
		}
	}

	this->curframe = func;//fun._function;

	Value temp = this->Execute(iptr);
	if (callstack.size() > 0)
		callstack.Pop();
	return temp;
};

Value& JetContext::operator[](const char* id)
{
	if (variables.find(id) == variables.end())
	{
		//add it
		variables[id] = variables.size();
		vars.push_back(Value());
	}
	return vars[variables[id]];
}

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

	Value v = this->Assemble(asmb);

	if (this->labels.size() > 100000)
		throw CompilerException("test", 5, "problem with labels!");

	return v;
}

void JetContext::Return(Value val)
{
	this->stack.Push(val);
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