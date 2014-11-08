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
	context->RunGC2();
};

void Jet::print(JetContext* context,Value* args, int numargs) 
{ 
	for (int i = 0; i < numargs; i++)
	{
		printf("%s", args[i].ToString().c_str());
	}
	printf("\n");
};

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
	(*this)["setprototype"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args != 2)
			throw RuntimeException("Invalid Call, Improper Arguments!");

		if (v->type == ValueType::Object && (v+1)->type == ValueType::Object)
		{
			Value val = *v;
			val._obj.prototype = (v+1)->_obj._object;
			context->Return(val);
		}
		else
		{
			throw RuntimeException("Improper arguments!");
		}
	});


	this->file.ptr = new std::map<std::string, Value>;
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
	this->string.ptr = new std::map<std::string, Value>;
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
			context->Return(Value((double)strlen(v->_obj._string)));
		else
			throw RuntimeException("bad length call!");
	});

	this->Array.ptr = new std::map<std::string, Value>;
	(*this->Array.ptr)["add"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 2)
		{
			(v+1)->_obj._array->ptr->push_back(*v);
			//(*(v+1)->_obj._array->ptr)[(v+1)->_obj._array->ptr->size()] = *(v);
		}
		else
			throw RuntimeException("Invalid add call!!");
	});
	(*this->Array.ptr)["size"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 1)
			context->Return((int)v->_obj._array->ptr->size());
		else
			throw RuntimeException("Invalid size call!!");
	});
	(*this->Array.ptr)["resize"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 2)
			(v+1)->_obj._array->ptr->resize((*v).value);
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
			it->container = v->_obj._array->ptr;
			it->iterator = v->_obj._array->ptr->begin();
			context->Return(context->NewUserdata(it, &context->arrayiter));
			return;
		}
		throw RuntimeException("Bad call to getIterator");
	});
	this->object.ptr = new std::map<std::string, Value>;
	(*this->object.ptr)["size"] = Value([](JetContext* context, Value* v, int args)
	{
		//how do I get access to the array from here?
		if (args == 1)
			context->Return((int)v->_obj._object->ptr->size());
		else
			throw RuntimeException("Invalid size call!!");
	});

	(*this->object.ptr)["getIterator"] = Value([](JetContext* context, Value* v, int args)
	{
		if (args == 1)
		{
			struct iter2
			{
				std::map<std::string, Value>* container;
				std::map<std::string, Value>::iterator iterator;
			};
			iter2* it = new iter2;
			it->container = v->_obj._object->ptr;
			it->iterator = v->_obj._object->ptr->begin();
			context->Return(context->NewUserdata(it, &context->objectiter));
			return;
		}
		throw RuntimeException("Bad call to getIterator");
	});
	this->objectiter.ptr = new std::map<std::string, Value>;
	(*this->objectiter.ptr)["next"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter2
		{
			std::map<std::string, Value>* container;
			std::map<std::string, Value>::iterator iterator;
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
			std::map<std::string, Value>* container;
			std::map<std::string, Value>::iterator iterator;
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
			std::map<std::string, Value>* container;
			std::map<std::string, Value>::iterator iterator;
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
			std::map<std::string, Value>* container;
			std::map<std::string, Value>::iterator iterator;
		};
		delete v->GetUserdata<iter2>();
	});
	this->arrayiter.ptr = new std::map<std::string, Value>;
	(*this->arrayiter.ptr)["next"] = Value([](JetContext* context, Value* v, int args)
	{
		struct iter
		{
			std::vector<Value>* container;
			std::vector<Value>::iterator iterator;
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
			std::vector<Value>* container;
			std::vector<Value>::iterator iterator;
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
			std::vector<Value>* container;
			std::vector<Value>::iterator iterator;
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
			std::vector<Value>* container;
			std::vector<Value>::iterator iterator;
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

void JetContext::RunGC2()
{
	//printf("Running GC: %d Greys, %d Globals, %d Stack\n%d Closures, %d Arrays, %d Objects, %d Userdata\n", this->greys.size(), this->vars.size(), 0, this->closures.size(), this->arrays.size(), this->objects.size(), this->userdata.size());
#ifdef JET_TIME_EXECUTION
	INT64 start, end;
	//QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif
	//add more write barriers to detect when objects are removed and what not
	//currently incremental collections dont do anything!
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
				if (vars[i]._obj._object->mark == false && vars[i]._obj._object->grey == false)
				{
					this->vars[i]._obj._object->grey = true;
					this->greys.Push(this->vars[i]);
				}
			}
		}
	}

	bool nextIncremental = ((this->collectionCounter+1)%GC_STEPS)!=0;
	//if (this->collectionCounter % GC_STEPS == 0)
	//printf("Full Collection!\n");
	//else
	//printf("Incremental Collection!\n");

	if (this->stack.size() > 0)
	{
		StackProfile prof("Make Stack Grey");
		for (int i = 0; i < this->stack.size(); i++)
		{
			if (stack.mem[i].type > ValueType::NativeFunction)
			{
				if (stack.mem[i]._obj._object->mark == false && stack.mem[i]._obj._object->grey == false)
				{
					stack.mem[i]._obj._object->grey = true;
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
		Closure* frame = this->curframe;
		for (int i = fptr; i >= 0; i--)
		{
			if (frame != 0)
				frame->mark;//break;

			//frame->mark = true;
			//get actual number of locals here from current frame
			for (int l = 0; l < this->frames[i].size; l++)
			{
				if (this->frames[i].locals[l].type > ValueType::NativeFunction)
				{
					if (this->frames[i].locals[l]._obj._object->mark == false && this->frames[i].locals[l]._obj._object->grey == false)
					{
						this->frames[i].locals[l]._obj._object->grey = true;
						this->greys.Push(this->frames[i].locals[l]);
					}
				}
			}
			frame = this->curframe->prev;
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
					if (obj._obj.prototype)
					{
						obj._obj.prototype->mark = true;

						for (auto ii: *obj._obj.prototype->ptr)
							greys.Push(ii.second);
					}

					obj._obj._object->mark = true;
					for (auto ii: *obj._obj._object->ptr)
					{
						if (ii.second.type > ValueType::NativeFunction && ii.second._obj._object->grey == false)
						{
							ii.second._obj._object->grey = true;
							greys.Push(ii.second);
						}
					}
					break;
				}
			case ValueType::Array:
				{
					obj._obj._array->mark = true;

					for (auto ii: *obj._obj._array->ptr)
					{
						if (ii.type > ValueType::NativeFunction && ii._obj._object->grey == false)
						{
							ii._obj._object->grey = true;
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

					if (obj._function->prev)
						obj._function->prev->mark = true;//make me loop through all parents

					for (int i = 0; i < obj._function->numupvals; i++)
					{
						greys.Push(obj._function->upvals[i]);
					}
					break;
				}
			case ValueType::Userdata:
				{
					obj._obj._userdata->mark = true;

					if (obj._obj.prototype)
					{
						obj._obj.prototype->mark = true;

						for (auto ii: *obj._obj.prototype->ptr)
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
	//printf("GC Complete: %d Greys, %d Globals, %d Stack\n%d Closures, %d Arrays, %d Objects, %d Userdata\n", this->greys.size(), this->vars.size(), 0, this->closures.size(), this->arrays.size(), this->objects.size(), this->userdata.size());
}

void JetContext::RunGC()
{
#ifdef JET_TIME_EXECUTION
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif

	for (auto ii: this->gcObjects)
		ii[0] = 0;

	for (auto& ii: this->objects)
		ii->mark = false;

	for (auto& ii: this->arrays)
		ii->mark = false;

	for (auto& ii: this->userdata)
		ii->mark = false;

	for (auto& ii: this->closures)
		ii->mark = false;

	//mark stack and locals here
	if (this->fptr >= 0)
	{
		//printf("GC run at runtime!\n");
		Closure* frame = this->curframe;
		for (int i = fptr; i >= 0; i--)
		{
			frame->mark = true;
			//get actual number of locals here from current frame
			for (int l = 0; l < frame->prototype->locals; l++)
			{
				if (this->frames[i].locals[l].type > ValueType::NativeFunction)
					this->frames[i].locals[l]._obj._object->mark = true;

				/*if (this->frames[i].locals[l].type == ValueType::Object)
				{
				this->frames[i].locals[l]._obj._object->mark = true;
				}
				else if (this->frames[i].locals[l].type == ValueType::Userdata)
				{
				this->frames[i].locals[l]._obj._userdata->mark = true;
				}
				else if (this->frames[i].locals[l].type == ValueType::Array)
				{
				this->frames[i].locals[l]._obj._array->mark = true;
				}
				else if (this->frames[i].locals[l].type == ValueType::Function)
				{
				this->frames[i].locals[l]._function->mark = true;
				}*/
				//else if (this->frames[i].locals[i].type == ValueType::String)
				//this->frames[i].locals[l]._obj._string->flag = true;
			}
			frame = this->curframe->prev;
		}
	}

	for (auto v: this->vars)
	{
		std::stack<Value*> stack;
		stack.push(&v);
		while( !stack.empty() ) 
		{
			auto o = stack.top();
			stack.pop();
			switch (o->type)
			{
			case ValueType::Object:
				{
					if (o->_obj.prototype)
					{
						o->_obj.prototype->mark = true;

						for (auto ii: *o->_obj.prototype->ptr)
							stack.push(&ii.second);
					}

					o->_obj._object->mark = true;
					for (auto ii: *o->_obj._object->ptr)
						stack.push(&ii.second);
					break;
				}
			case ValueType::Array:
				{
					o->_obj._array->mark = true;

					for (auto ii: *o->_obj._array->ptr)
						stack.push(&ii);

					break;
				}
			case ValueType::String:
				{
					break;
				}
			case ValueType::Function:
				{
					o->_function->mark = true;

					if (o->_function->prev)
						o->_function->prev->mark = true;//make me loop through all parents

					break;
				}
			case ValueType::Userdata:
				{
					o->_obj._userdata->mark = true;

					if (o->_obj.prototype)
					{
						o->_obj.prototype->mark = true;

						for (auto ii: *o->_obj.prototype->ptr)
							stack.push(&ii.second);
					}
					break;
				}
			}
		}
	}

	for (auto& ii: this->objects)
	{
		if (!ii->mark)
		{
			delete ii->ptr;
			ii->ptr = 0;
			delete ii;
			ii = 0;
		}
	}

	for (auto& ii: this->arrays)
	{
		if (!ii->mark)
		{
			delete ii->ptr;
			ii->ptr = 0;
			delete ii;
			ii = 0;
		}
	}

	auto clist = std::move(this->closures);
	this->closures.clear();
	for (auto ii: clist)
	{
		if (ii->mark)
		{
			this->closures.push_back(ii);
		}
		else
		{
			if (ii->numupvals)
				delete[] ii->upvals;
			delete ii;
		}
	}

	//ok, need to be able to know the type of a gcobject, or just that its userdata
	for (auto& ii: this->gcObjects)
	{
		if (ii[0] == 0)
		{
			//printf("gcObject needs deletion");
		}
	}

	for (auto& ii: this->userdata)
	{
		if (!ii->mark)
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
			ii = 0;
		}
	}

	//compact object list here
	auto list = std::move(this->objects);
	this->objects.clear();
	for (auto ii: list)
	{
		if (ii)
		{
			this->objects.push_back(ii);
		}
	}

	//compact array list here
	auto arrlist = std::move(this->arrays);
	this->arrays.clear();
	for (auto ii: arrlist)
	{
		if (ii)
		{
			this->arrays.push_back(ii);
		}
	}

	//compact userdata list here
	auto udlist = std::move(this->userdata);
	this->userdata.clear();
	for (auto ii: udlist)
	{
		if (ii)
		{
			this->userdata.push_back(ii);
		}
	}

#ifdef JET_TIME_EXECUTION
	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to collect garbage\n\n", dt);
#endif
}

Value JetContext::Execute(int iptr)
{
#ifdef JET_TIME_EXECUTION
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif

	//frame pointer reset
	fptr = 0;

	callstack.Push(std::pair<unsigned int, Closure*>(123456789, curframe));//bad value to get it to return;
	unsigned int startcallstack = this->callstack.size();

	this->frames[0].size = curframe->prototype->locals;//used in gc

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

					if ((double)one == (double)two)
						stack.Push(Value(1));
					else
						stack.Push(Value(0));

					break;
				}
			case InstructionType::NotEq:
				{
					Value one = stack.Pop();
					Value two = stack.Pop();

					if ((double)one == (double)two)
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
					stack.Push(in.value);
					break;
				}
			case InstructionType::LdStr:
				{
					stack.Push(in.string);
					break;
				}
			case InstructionType::Jump:
				{
					iptr = (int)in.value-1;
					break;
				}
			case InstructionType::JumpTrue:
				{
					auto temp = stack.Pop();
					if ((int)temp)
						iptr = (int)in.value-1;
					break;
				}
			case InstructionType::JumpFalse:
				{
					auto temp = stack.Pop();
					if (!(int)temp)
						iptr = (int)in.value-1;
					break;
				}
			case InstructionType::Load:
				{
					stack.Push(vars[(int)in.value]);//uh do me

					break;
				}
			case InstructionType::LLoad:
				{
					stack.Push(frames[fptr].locals[(unsigned int)in.value]);
					break;
				}
			case InstructionType::LStore:
				{
					frames[fptr].locals[(unsigned int)in.value] = stack.Pop();
					break;
				}
			case InstructionType::CLoad:
				{
					auto frame = curframe;
					int index = in.value2;
					while ( index++ < 0)
						frame = frame->prev;

					stack.Push(frame->upvals[(int)in.value]);
					break;
				}
			case InstructionType::CStore:
				{
					auto frame = curframe;
					int index = in.value2;
					while ( index++ < 0)
						frame = frame->prev;

					frame->upvals[(int)in.value] = stack.Pop();
					break;
				}
			case InstructionType::LoadFunction:
				{
					//construct a new closure with the right number of upvalues
					//from the Func* object
					Closure* closure = new Closure;
					closure->prev = curframe;
					closure->numupvals = in.func->upvals;
					if (in.func->upvals)
						closure->upvals = new Value[in.func->upvals];
					closure->prototype = in.func;
					this->closures.push_back(closure);
					stack.Push(Value(closure));
					break;
				}
			case InstructionType::Store:
				{
					auto temp = stack.Pop();
					//store me
					vars[(int)in.value] = temp;
					break;
				}
			case InstructionType::Call:
				{
					//need to store how many args each function takes in the bytecode
					//change function calls to automatically push args into locals, and take account for extra/missing values
					if (vars[(int)in.value].type == ValueType::Function)
					{
						//store iptr on call stack
						if (fptr > 38)
							throw RuntimeException("Stack Overflow!");

						fptr++;

						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));//callstack.Push(iptr);

						curframe = vars[(int)in.value]._function;

						frames[fptr].size = curframe->prototype->locals;

						Function* func = vars[(int)in.value]._function->prototype;
						//set all the locals
						if (in.value2 <= func->args)
						{
							for (int i = func->args-1; i >= 0; i--)
							{
								if (i < (int)in.value2)
									frames[fptr].locals[i] = stack.Pop();
								else
									frames[fptr].locals[i] = Value();
							}
						}
						else
						{
							for (int i = (int)in.value2-1; i >= 0; i--)
							{
								if (i < func->args)
									frames[fptr].locals[i] = stack.Pop();
								else
									stack.Pop();
							}
						}

						//go to function
						iptr = func->ptr-1;
					}
					else if (vars[(int)in.value].type == ValueType::NativeFunction)
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
						(*vars[(int)in.value].func)(this,tmp,args);

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
							if (ii.second == (unsigned int)in.value)
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
					Value fun = stack.Pop();
					if (fun.type == ValueType::Function)
					{
						if (fptr > 38)
							throw RuntimeException("Stack Overflow!");

						fptr++;
						//callstack.Push(iptr);
						callstack.Push(std::pair<unsigned int, Closure*>(iptr, curframe));

						curframe = fun._function;

						frames[fptr].size = curframe->prototype->locals;

						Function* func = fun._function->prototype;
						//set all the locals
						if (in.value <= func->args)
						{
							for (int i = func->args-1; i >= 0; i--)
							{
								if (i < (int)in.value)
									frames[fptr].locals[i] = stack.Pop();
								else
									frames[fptr].locals[i] = Value();
							}
						}
						else
						{
							for (int i = (int)in.value-1; i >= 0; i--)
							{
								if (i < func->args)
									frames[fptr].locals[i] = stack.Pop();
								else
									stack.Pop();
							}
						}
						//use the new Function* struct
						//go to function
						iptr = fun._function->prototype->ptr-1;
						//iptr = fun.ptr-1;
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
							(*loc._obj._object->ptr)[in.string] = val;
						else
							throw RuntimeException("Could not index a non array/object value!");

						if (loc._obj._object->mark)
						{
							//reset to grey and push back for reprocessing
							//printf("write barrier triggered!\n");
							loc._obj._object->mark = false;
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
							if ((int)index >= loc._obj._array->ptr->size() || (int)index < 0)
								throw RuntimeException("Array index out of range!");
							(*loc._obj._array->ptr)[(int)index] = val;

							//write barrier
							if (loc._obj._array->mark)
							{
								//reset to grey and push back for reprocessing
								//printf("write barrier triggered!\n");
								loc._obj._array->mark = false;
								this->greys.Push(loc);//push to grey stack
							}
						}
						else if (loc.type == ValueType::Object)
						{
							(*loc._obj._object->ptr)[index.ToString()] = val;

							//write barrier
							if (loc._obj._object->mark)
							{
								//reset to grey and push back for reprocessing
								//printf("write barrier triggered!\n");
								loc._obj._object->mark = false;
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

						if (loc.type == ValueType::Object)
						{
							Value v;
							auto ii = loc._obj._object->ptr->find(in.string);
							if (ii == loc._obj._object->ptr->end())
							{
								if (loc._obj.prototype)
								{
									ii = loc._obj.prototype->ptr->find(in.string);
									if (ii == loc._obj.prototype->ptr->end())
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
							//Value v = (*loc._obj._object->ptr)[in.string];

							//check meta table
							//if (v.type == ValueType::Null && loc._obj.prototype)
							//v = (*loc._obj.prototype->ptr)[in.string];
							//if (v.type == ValueType::Null)
							//v = (*this->object.ptr)[in.string];
							//stack.Push(v);
						}
						else if (loc.type == ValueType::String)
							stack.Push((*this->string.ptr)[in.string]);
						else if (loc.type == ValueType::Array)
							stack.Push((*this->Array.ptr)[in.string]);
						else if (loc.type == ValueType::Userdata)
							stack.Push((*loc._obj.prototype->ptr)[in.string]);
						else
							throw RuntimeException("Could not index a non array/object value!");
					}
					else
					{
						Value index = stack.Pop();
						Value loc = stack.Pop();

						if (loc.type == ValueType::Array)
						{
							if ((int)index >= loc._obj._array->ptr->size())
								throw RuntimeException("Array index out of range!");
							stack.Push((*loc._obj._array->ptr)[(int)index]);
						}
						else if (loc.type == ValueType::Object)
							stack.Push((*loc._obj._object->ptr)[index.ToString()]);
						else
							throw RuntimeException("Could not index a non array/object value!");
					}
					break;
				}
			case InstructionType::NewArray:
				{
					//need to call gc sometime
					auto arr = new GCVal<std::vector<Value>*>(new std::vector<Value>(in.value));//new std::map<int, Value>;
					arr->grey = false;
					arr->mark = false;
					this->arrays.push_back(arr);
					for (int i = (int)in.value-1; i >= 0; i--)
						(*arr->ptr)[i] = stack.Pop();
					stack.Push(Value(arr));

					if (allocationCounter++%GC_INTERVAL == 0)
						this->RunGC2();

					break;
				}
			case InstructionType::NewObject:
				{
					//need to call gc sometime
					auto obj = new GCVal<std::map<std::string, Value>*>(new std::map<std::string, Value>);
					obj->grey = false;
					obj->mark = false;
					this->objects.push_back(obj);
					for (int i = (int)in.value-1; i >= 0; i--)
						(*obj->ptr)[stack.Pop().ToString()] = stack.Pop();
					stack.Push(Value(obj));

					if (allocationCounter++%GC_INTERVAL == 0)
						this->RunGC2();

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
				Value v = frames[fptr].locals[i];
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
		this->stack.QuickPop(this->stack.size());
		this->callstack.QuickPop(this->callstack.size());

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
				//do something with argument and local counts
				Function* func = new Function;
				func->args = inst.a;
				func->locals = inst.b;
				func->upvals = inst.c;
				func->ptr = labelposition;
				func->name = inst.string;

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
			{
				break;
			}
		case InstructionType::Function:
			{
				break;
			}
		case InstructionType::Label:
			{
				break;
			}
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

				if (inst.type == InstructionType::Store)
				{
					if (variables.find(inst.string) == variables.end())
					{
						//add it
						variables[inst.string] = variables.size();
						vars.push_back(Value());
					}
					ins.value = variables[inst.string];
				}
				else if (inst.type == InstructionType::Load)
				{
					if (variables.find(inst.string) == variables.end())
					{
						//add it
						variables[inst.string] = variables.size();
						vars.push_back(Value());
					}
					ins.value = variables[inst.string];
				}
				else if (inst.type == InstructionType::LoadFunction)
				{
					ins.func = functions[inst.string];
				}
				else if (inst.type == InstructionType::Call)
				{
					if (variables.find(inst.string) == variables.end())
					{
						//add it
						variables[inst.string] = variables.size();
						vars.push_back(Value());
					}
					//make sure to point to the right variable
					ins.value = variables[inst.string];
				}
				else if (inst.type == InstructionType::Jump || inst.type == InstructionType::JumpFalse || inst.type == InstructionType::JumpTrue)
				{
					ins.value = labels[inst.string];
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
	tmpframe->prototype = this->functions["{Entry Point}"];
	tmpframe->numupvals = tmpframe->prototype->upvals;
	if (tmpframe->numupvals)
		tmpframe->upvals = new Value[tmpframe->numupvals];
	else
		tmpframe->upvals = 0;

	this->curframe = tmpframe;

	this->closures.push_back(tmpframe);

	Value temp = this->Execute(tmpframe->prototype->ptr);//run the static code
	if (this->callstack.size() > 0)
		this->callstack.Pop();

	return temp;
};

//executes a function in the VM context
Value JetContext::Call(const char* function, Value* args, unsigned int numargs)
{
	int iptr = 0;
	printf("Calling: '%s'\n", function);
	if (variables.find(function) == variables.end())
	{
		printf("ERROR: No variable named: '%s' to call\n", function);
		return Value(0);
	}

	Value func = vars[variables[function]];
	if (func.type != ValueType::NativeFunction && func.type != ValueType::Function)
	{
		printf("ERROR: Variable '%s' is not a function\n", function);
		return Value(0);
	}
	else if (func.type == ValueType::NativeFunction)
	{
		//call it
		(*func.func)(this,args,numargs);
		return Value(0);
	}
	iptr = func._function->prototype->ptr;

	//push args onto stack
	for (unsigned int i = 0; i < numargs; i++)
	{
		this->stack.Push(args[i]);
	}

	this->curframe = func._function;

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
		vars.push_back(0);
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
	catch(ParserException E)
	{
		printf("Exception found:\n");
		printf("%s (%d): %s\n", E.file.c_str(), E.line, E.ShowReason());
	}
}

Value JetContext::Script(const char* code, const char* filename)//compiles, assembles and executes the script
{
	auto asmb = this->Compile(code, filename);

	Value v = this->Assemble(asmb);

	//this->RunGC();
	if (this->labels.size() > 100000)
		throw CompilerException("test", 5, "problem with lables!");
	return v;
}

#ifdef EMSCRIPTEN
#ifndef _HGUAD
#define _HGUAD
#include <emscripten/bind.h>
using namespace emscripten;
//using namespace std;

EMSCRIPTEN_BINDINGS(Jet) {
	class_<Jet::JetContext>("JetContext")
		.constructor()
		.function<std::string, std::string, std::string>("Script", &JetContext::Script);
}
#endif
#endif