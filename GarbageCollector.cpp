#include "GarbageCollector.h"
#include "JetContext.h"

using namespace Jet;

GarbageCollector::GarbageCollector(JetContext* context) : context(context)
{
	this->allocationCounter = 1;//messes up if these start at 0
	this->collectionCounter = 1;
}

void GarbageCollector::Cleanup()
{
	for (auto ii: this->userdata)
	{
		if (ii->ptr.second)
		{
			Value ud = Value(ii, ii->ptr.second);
			Value _gc = (*ii->ptr.second)["_gc"];
			if (_gc.type == ValueType::NativeFunction)
				_gc.func(this->context, &ud, 1);
			else if (_gc.type == ValueType::Function)
				throw RuntimeException("Non Native _gc Hooks Not Implemented!");//todo
			else if (_gc.type != ValueType::Null)
				throw RuntimeException("Invalid _gc Hook!");
		}
		delete ii;
	}
	this->userdata.clear();

	for (auto ii: this->arrays)
	{
		delete ii->ptr;
		delete ii;
	}
	this->arrays.clear();

	for (auto ii: this->objects)
	{
		delete ii;
	}
	this->objects.clear();

	for (auto ii: this->strings)
	{
		delete ii->ptr;
		delete ii;
	}
	this->strings.clear();

	for (auto ii: this->closures)
	{
		if (ii->numupvals)
			delete[] ii->upvals;
		delete ii;
	}
	this->closures.clear();
}

void GarbageCollector::Mark()
{
	//mark basic types
	this->greys.Push(context->Array);
	this->greys.Push(context->arrayiter);
	this->greys.Push(context->object);
	this->greys.Push(context->objectiter);
	this->greys.Push(context->string);
	this->greys.Push(context->file);


	//mark all objects being held by native code
	for (int i = 0; i < this->nativeRefs.size(); i++)
	{
		if (this->nativeRefs[i]._object->grey == false)
		{
			this->nativeRefs[i]._object->grey = true;
			this->greys.Push(this->nativeRefs[i]);
		}
	}

	//add more write barriers to detect when objects are removed and what not
	//if flag is marked, then black
	//if no flag and grey bit, then grey
	//if no flag or grey bit, then white

	//push all reachable items onto grey stack
	//this means globals
	{
		//StackProfile profile("Mark Globals as Grey");
		for (int i = 0; i < context->vars.size(); i++)
		{
			if (context->vars[i].type > ValueType::NativeFunction)
			{
				if (context->vars[i]._object->grey == false)
				{
					context->vars[i]._object->grey = true;
					this->greys.Push(context->vars[i]);
				}
			}
		}
	}


	if (context->stack.size() > 0)
	{
		//StackProfile prof("Make Stack Grey");
		for (int i = 0; i < context->stack.size(); i++)
		{
			if (context->stack.mem[i].type > ValueType::NativeFunction)
			{
				if (context->stack.mem[i]._object->grey == false)
				{
					context->stack.mem[i]._object->grey = true;
					this->greys.Push(context->stack.mem[i]);
				}
			}
		}
	}


	//this is really part of the sweep section
	if (context->callstack.size() > 0)
	{
		//StackProfile prof("Traverse/Mark Stack");
		//printf("GC run at runtime!\n");
		//traverse all local vars
		if (context->curframe->grey == false)
		{
			context->curframe->grey = true;
			this->greys.Push(Value(context->curframe));
		}

		int sp = 0;
		for (int i = 0; i < context->callstack.size(); i++)
		{
			auto closure = context->callstack[i].second;
			if (closure == 0)
				continue;

			if (closure->grey == false)
			{
				closure->grey = true;
				this->greys.Push(Value(closure));
			}
			int max = sp+closure->prototype->locals;
			for (; sp < max; sp++)
			{
				if (context->localstack[sp].type > ValueType::NativeFunction)
				{
					if (context->localstack[sp]._object->grey == false)
					{
						context->localstack[sp]._object->grey = true;
						this->greys.Push(context->localstack[sp]);
					}
				}
			}
		}

		//mark curframe locals
		int max = sp+context->curframe->prototype->locals;
		for (; sp < max; sp++)
		{
			if (context->localstack[sp].type > ValueType::NativeFunction)
			{
				if (context->localstack[sp]._object->grey == false)
				{
					context->localstack[sp]._object->grey = true;
					this->greys.Push(context->localstack[sp]);
				}
			}
		}
	}


	{
		//StackProfile prof("Traverse Greys");
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

						for (auto ii: *obj.prototype)
						{
							if (ii.first.type > ValueType::NativeFunction && ii.first._object->grey == false)
							{
								ii.first._object->grey = true;
								greys.Push(ii.first);
							}
							if (ii.second.type > ValueType::NativeFunction && ii.second._object->grey == false)
							{
								ii.second._object->grey = true;
								greys.Push(ii.second);
							}
						}
					}

					obj._object->mark = true;
					for (auto ii: *obj._object)
					{
						if (ii.first.type > ValueType::NativeFunction && ii.first._object->grey == false)
						{
							ii.first._object->grey = true;
							greys.Push(ii.first);
						}
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
					obj._string->mark = true;

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

					if (obj.prototype && obj.prototype->grey == false)
					{
						obj.prototype->grey = true;
						obj.prototype->mark = true;

						for (auto ii: *obj.prototype)
						{
							greys.Push(ii.first);
							greys.Push(ii.second);
						}
					}
					break;
				}
			}
		}
	}
}

void GarbageCollector::Sweep()
{
	bool nextIncremental = ((this->collectionCounter+1)%GC_STEPS)!=0;
	/*if (this->collectionCounter % GC_STEPS == 0)
	printf("Full Collection!\n");
	else
	printf("Incremental Collection!\n");*/

	/* SWEEPING SECTION */


	//this must all be done when sweeping!!!!!!!

	//process stack variables, stack vars are ALWAYS grey

	//finally sweep through
	//sweep and free all whites and make all blacks white
	//iterate through all gc values

	//mark stack variables and globals
	{
		//StackProfile prof("Sweep");
		auto alist = std::move(this->arrays);
		this->arrays.clear();
		for (auto ii: alist)
		{
			if (ii->mark || ii->refcount)
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
			if (ii->mark || ii->refcount)
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
			if (ii->mark || ii->refcount)
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
					Value _gc = (*ii->ptr.second)["_gc"];
					if (_gc.type == ValueType::NativeFunction)
						_gc.func(this->context, &ud, 1);
					else if (_gc.type != ValueType::Null)
						throw RuntimeException("Non-native Userdata Finalizers Not Implemented!");
				}
				delete ii;
			}
		}

		auto olist = std::move(this->objects);
		this->objects.clear();
		for (auto ii: olist)
		{
			if (ii->mark || ii->refcount)
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
				delete ii;
			}
		}

		auto slist = std::move(this->strings);
		this->strings.clear();
		for (auto ii: slist)
		{
			if (ii->mark || ii->refcount)
			{
				if (nextIncremental == false)
				{
					ii->mark = false;
					ii->grey = false;
				}
				this->strings.push_back(ii);
			}
			else
			{
				delete ii->ptr;
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
}

void GarbageCollector::Run()
{
	//printf("Running GC: %d Greys, %d Globals, %d Stack\n%d Closures, %d Arrays, %d Objects, %d Userdata\n", this->greys.size(), this->vars.size(), 0, this->closures.size(), this->arrays.size(), this->objects.size(), this->userdata.size());
#ifdef JET_TIME_EXECUTION
	INT64 start, end;
	//QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif
	//mark all references in the grey stack
	this->Mark();

	//clear up dead memory
	this->Sweep();

	this->collectionCounter++;//used to determine collection mode
#ifdef JET_TIME_EXECUTION
	INT64 rate;
	QueryPerformanceCounter( (LARGE_INTEGER *)&end );
	QueryPerformanceCounter((LARGE_INTEGER*)&rate);
	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to collect garbage\n\n", dt);
#endif
	//this->StackTrace(curframe->prototype->ptr);
	//printf("GC Complete: %d Greys, %d Globals, %d Stack\n%d Closures, %d Arrays, %d Objects, %d Userdata\n", this->greys.size(), this->vars.size(), 0, this->closures.size(), this->arrays.size(), this->objects.size(), this->userdata.size());
}