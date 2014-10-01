#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
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

std::vector<IntermediateInstruction> JetContext::Compile(const char* code)
{
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );

	Lexer lexer = Lexer(code);
	Parser parser = Parser(&lexer);

	//printf("In: %s\n\nResult:\n", code);
	BlockExpression* result = parser.parseAll();
	//result->print();
	//printf("\n\n");

	std::vector<IntermediateInstruction> out = compiler.Compile(result);

	delete result;

	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to compile\n\n", dt);

	return std::move(out);
}

void JetContext::RunGC()
{
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );
	for (auto& ii: this->objects)
	{
		ii->flag = false;
	}

	for (auto& ii: this->arrays)
	{
		ii->flag = false;
	}

	//mark stack and locals here
	if (this->fptr >= 0)
	{
		//printf("GC run at runtime!\n");
		for (int i = fptr; i >= 0; i--)
		{
			for (int l = 0; l < 20; l++)
			{
				if (this->frames[i].locals[l].type == ValueType::Object)
				{
					this->frames[i].locals[l]._obj->flag = true;
					//printf("marked a local!\n");
				}
			}
			//printf("1 stack frame\n");
		}
	}

	//check locals here
	if (this->stack.size() > 0)
	{
		int loc = this->stack.size();
		for (int i = 0; i < loc; i++)
		{
			if (this->stack.mem[i].type == ValueType::Object)
			{
				std::stack<Value*> stack;
				stack.push(&this->stack.mem[i]);
				while( !stack.empty() ) {
					auto o = stack.top();
					stack.pop();
					if (o->type == ValueType::Object)
					{
						o->_obj->flag = true;

						for (auto ii: *o->_obj->ptr)
							stack.push(&ii.second);
					}
					else if (o->type == ValueType::Array)
					{
						o->_array->flag = true;

						for (auto ii: *o->_array->ptr)
							stack.push(&ii.second);
					}
				}
			}
			else if (this->stack.mem[i].type == ValueType::Array)
			{
				std::stack<Value*> stack;
				stack.push(&this->stack.mem[i]);
				while( !stack.empty() ) {
					auto o = stack.top();
					stack.pop();
					if (o->type == ValueType::Object)
					{
						o->_obj->flag = true;

						for (auto ii: *o->_obj->ptr)
							stack.push(&ii.second);
					}
					else if (o->type == ValueType::Array)
					{
						o->_array->flag = true;

						for (auto ii: *o->_array->ptr)
							stack.push(&ii.second);
					}
				}
			}
		}
		printf("there was stack left");
	}

	for (auto v: this->vars)
	{
		std::stack<Value*> stack;
		stack.push(&v);
		while( !stack.empty() ) {
			auto o = stack.top();
			stack.pop();
			if (o->type == ValueType::Object)
			{
				o->_obj->flag = true;

				for (auto ii: *o->_obj->ptr)
					stack.push(&ii.second);
			}
			else if (o->type == ValueType::Array)
			{
				o->_array->flag = true;

				for (auto ii: *o->_array->ptr)
					stack.push(&ii.second);
			}
		}
	}

	for (auto& ii: this->objects)
	{
		if (!ii->flag)
		{
			//printf("deleted object!");
			delete ii->ptr;
			ii->ptr = 0;
			delete ii;
			ii = 0;
		}
	}

	for (auto& ii: this->arrays)
	{
		if (!ii->flag)
		{
			//printf("deleted array!");
			delete ii->ptr;
			ii->ptr = 0;
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

	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to collect garbage\n\n", dt);
}

Value JetContext::Execute(int iptr)
{
	INT64 start, rate, end;
	QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
	QueryPerformanceCounter( (LARGE_INTEGER *)&start );

	//frame pointer reset
	fptr = 0;

	callstack.Push(123456789);//bad value to get it to return;
	unsigned int startcallstack = this->callstack.size();

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
					stack.Push(frames[fptr].Get((unsigned int)in.value, (unsigned int)in.value2));
					break;
				}
			case InstructionType::LStore:
				{
					frames[fptr].Set(stack.Pop(), (unsigned int)in.value, (unsigned int)in.value2);
					break;
				}
			case InstructionType::LoadFunction:
				{
					stack.Push(Value((unsigned int)in.value, true));
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
					if (vars[(int)in.value].type == ValueType::Function)
					{
						//store iptr on call stack
						if (fptr > 38)
							throw JetRuntimeException("Stack Overflow!");

						fptr++;
						callstack.Push(iptr);
						//go to function
						iptr = (int)vars[(int)in.value].ptr-1;//in.value-1;
					}
					else if (vars[(int)in.value].type == ValueType::NativeFunction)
					{
						unsigned int args = (unsigned int)in.value2;
						Value* tmp = &stack.mem[stack.size()-args];//new Value[(unsigned int)in.value2];
						stack.QuickPop(args);//pop off args
						/*for (int i = ((int)in.value2)-1; i >= 0; i--)
						{
						stack.Pop();
						}*/
						//ok fix this to be cleaner and resolve stack printing
						//should just push a value to indicate that we are in a native function call
						//fixme this stuff is baaaad
						callstack.Push(iptr);
						callstack.Push(123456789);
						//to return something, push it to the stack
						int s = stack.size();
						(*vars[(int)in.value].func)(this,tmp,args);

						callstack.QuickPop(2);
						if (stack.size() == s)//we didnt return anything
							stack.Push(Value());//return null
						//delete[] tmp;
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
						throw JetRuntimeException("Cannot call non function type '" + var + "'!!!");
					}

					break;
				}
			case InstructionType::ECall:
				{
					Value fun = stack.Pop();

					if (fun.type == ValueType::Function)
					{
						if (fptr > 38)
							throw JetRuntimeException("Stack Overflow!");

						fptr++;
						callstack.Push(iptr);
						//go to function
						iptr = fun.ptr-1;//in.value-1;
					}
					else if (vars[(int)in.value].type == ValueType::NativeFunction)
					{
						throw JetRuntimeException("Not Implemented!!!");
					}
					else
					{
						throw JetRuntimeException("Cannot call non function type stack_value!!!");
					}
					break;
				}
			case InstructionType::Return:
				{
					//get iptr from stack
					//if (callstack.size() == 1)
					//iptr = 55555555;
					//else
					iptr = callstack.Pop();

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
						//Value index = stack.Pop();
						Value loc = stack.Pop();
						Value val = stack.Pop();	

						//if (loc.type == ValueType::Array)
						//(*loc._array->ptr)[(int)index] = val;
						if (loc.type == ValueType::Object)
							(*loc._obj->ptr)[in.string] = val;
						else
							throw JetRuntimeException("Could not index a non array/object value!");
					}
					else
					{
						Value index = stack.Pop();
						Value loc = stack.Pop();
						Value val = stack.Pop();	

						if (loc.type == ValueType::Array)
							(*loc._array->ptr)[(int)index] = val;
						else if (loc.type == ValueType::Object)
							(*loc._obj->ptr)[index.ToString()] = val;
						else
							throw JetRuntimeException("Could not index a non array/object value!");
					}
					break;
				}
			case InstructionType::LoadAt:
				{
					if (in.string)
					{
						//Value index = stack.Pop();
						Value loc = stack.Pop();

						if (loc.type == ValueType::Object)
							stack.Push((*loc._obj->ptr)[in.string]);
						else
							throw JetRuntimeException("Could not index a non array/object value!");
					}
					else
					{
						Value index = stack.Pop();
						Value loc = stack.Pop();

						if (loc.type == ValueType::Array)
							stack.Push((*loc._array->ptr)[(int)index]);
						else if (loc.type == ValueType::Object)
							stack.Push((*loc._obj->ptr)[index.ToString()]);
						else
							throw JetRuntimeException("Could not index a non array/object value!");
					}
					break;
				}
			case InstructionType::NewArray:
				{
					auto arr = new GCVal<std::map<int, Value>*>(new std::map<int, Value>);//new std::map<int, Value>;
					this->arrays.push_back(arr);
					for (int i = (int)in.value-1; i >= 0; i--)
						(*arr->ptr)[i] = stack.Pop();
					stack.Push(Value(arr));
					break;
				}
			case InstructionType::NewObject:
				{
					auto obj = new GCVal<std::map<std::string, Value>*>(new std::map<std::string, Value>);
					this->objects.push_back(obj);
					for (int i = (int)in.value-1; i >= 0; i--)
						(*obj->ptr)[stack.Pop().ToString()] = stack.Pop();
					stack.Push(Value(obj));
					break;
				}
			default:
				{
					throw JetRuntimeException("Unimplemented Instruction!");
				}
			}

			iptr++;
		}
	}
	catch(JetRuntimeException e)
	{
		printf("RuntimeException: %s\nCallstack:\n", e.reason.c_str());

		//generate call stack
		this->StackTrace(iptr);

		printf("\nLocals:\n");
		for (int i = 0; i < 2; i++)
		{
			Value v = frames[fptr].locals[i];
			if (v.type >= ValueType(0))
				printf("%d = %s\n", i, v.ToString().c_str());
		}

		printf("\nVariables:\n");
		for (auto ii: variables)
		{
			printf("%s = %s\n", ii.first.c_str(), vars[ii.second].ToString().c_str());
		}

		//ok, need to properly roll back callstack
		this->callstack.QuickPop(this->callstack.size()-startcallstack);

		if (this->callstack.size() == 1 && this->callstack.Peek() == 123456789)
			this->callstack.Pop();
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

		if (this->callstack.size() == 1 && this->callstack.Peek() == 123456789)
			this->callstack.Pop();
	}

	QueryPerformanceCounter( (LARGE_INTEGER *)&end );

	INT64 diff = end - start;
	double dt = ((double)diff)/((double)rate);

	printf("Took %lf seconds to execute\n\n", dt);

	this->fptr = -1;//set to invalid to indicate to the GC that we arent executing if it gets ran

	/*printf("Variables:\n");
	for (auto ii: variables)
	{
	printf("%s = %s\n", ii.first.c_str(), vars[ii.second].ToString().c_str());
	}*/

	/*printf("\nLabels:\n");
	for (auto ii: labels)
	{
	printf("%s = %d\n", ii.first.c_str(), ii.second);
	}*/

	if (stack.size() == 0)
	{
		return Value();
	}

	return stack.Pop();
}
