#ifndef _LANG_CONTEXT_HEADER
#define _LANG_CONTEXT_HEADER

#include <functional>
#include <string>

static enum InstructionType
{
	Add,
	Mul,
	Div,
	Sub,
	Modulus,

	Eq,
	NotEq,
	Lt,
	Gt,

	Incr,
	Decr,

	Dup,
	Pop,

	LdNum,
	LdStr,
	LoadFunction,

	Jump,
	JumpTrue,
	JumpFalse,

	Store,
	Load,

	//these all work on the last value in the stack
	EStore,
	ELoad,
	ECall,

	Call,
	Return
};

class RuntimeException
{
public:
	std::string reason;
	RuntimeException(std::string reason)
	{
		this->reason = reason;
	}
};

struct Instruction
{
	InstructionType instruction;
	double value;
	union
	{
		double value2;
		const char* string;
	};
};

enum class ValueType
{
	Number = 0,
	String,
	Object,
	Function,
	NativeFunction,
	Userdata,
};

class LangContext;

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
		void* object;//used for userdata
		unsigned int ptr;
		std::function<void(LangContext*,Value*,int)>* func;
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

	Value(std::function<void(LangContext*,Value*,int)>* a)
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
			return std::to_string(this->value);
		case ValueType::String:
			return this->string.data;
		case ValueType::Function:
			return "[Function "+std::to_string(this->ptr)+"]"; 
		case ValueType::NativeFunction:
			return "[NativeFunction "+std::to_string((unsigned int)this->func)+"]";
		}
	}

	operator int()
	{
		if (type == ValueType::Number)
			return (int)value;

		return 0;
	}

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

	//Value operator() (/*args*/)
	//{
	//cant really do anything in here
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

#include "VMStack.h"
#include "Parser.h"
#include <Windows.h>
class LangContext
{
	VMStack<Value> stack;
	VMStack<unsigned int> callstack;

	//need to go through and find all labels/functions/variables
	std::map<std::string, unsigned int> labels;
	std::map<std::string, unsigned int> functions;
	std::map<std::string, unsigned int> variables;//mapping from string to location in vars array

	//actual data being worked on
	std::vector<Instruction> ins;
	std::vector<Value> vars;//where they are actually stored

	int labelposition;

	CompilerContext compiler;
public:
	LangContext()
	{
		this->labelposition = 0;
		stack = VMStack<Value>(500000);	
	};

	//allows assignment and reading of variables stored
	Value& operator[](const char* id)
	{
		if (variables.find(id) == variables.end())
		{
			//add it
			variables[id] = variables.size();
			vars.push_back(0);
		}
		return vars[variables[id]];
	}

	Value Script(const char* code)//compiles, assembles and executes the script
	{
		const char* asmb = this->Compile(code);
		Value v = this->Assemble(asmb);
		delete[] asmb;
		return v;
	}

	//compiles source code to ASM for the VM to read in
	const char* Compile(const char* code)
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

		std::string out = compiler.Compile(result);

		delete result;

		QueryPerformanceCounter( (LARGE_INTEGER *)&end );

		INT64 diff = end - start;
		double dt = ((double)diff)/((double)rate);

		printf("Took %lf seconds to compile\n\n", dt);

		char* nout = new char[out.size()+1];
		out.copy(nout, out.size(), 0);
		nout[out.size()] = 0;
		return nout;
	}

	//puts compiled code into the VM and runs any globals
	Value Assemble(const char* code)//takes in assembly code for execution
	{
		INT64 start, rate, end;
		QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
		QueryPerformanceCounter( (LARGE_INTEGER *)&start );

		//O(2n) complexity
		const char* tmp = code;
		//this loop subs constant string names for integer var array indices and reads in labels
		while(*code != 0)
		{
			if (*code == '\n' && *(code+1) == 0)
				break;

			char instruction[40];
			char name[40];
			double value;
			sscanf(code, "%s %s", instruction, name);

			if (instruction[strlen(instruction)-1] == ':')//its a label
			{
				//just label
				instruction[strlen(instruction)-1] = 0;
				if (labels.find(instruction) == labels.end())
					labels[instruction] = labelposition;
				else
				{
					printf("ERROR: Duplicate Label Name: %s\n", instruction);
				}
			}
			else if (strncmp(instruction, "func", 5) == 0 && name[strlen(name)-1] == ':')
			{
				//look for function definitions
				//need to push something about the function to variables somewhere, preferably in code

				//it is function
				name[strlen(name)-1] = 0;
				if (functions.find(name) == functions.end())
					functions[name] = labelposition;
				else
				{
					printf("ERROR: Duplicate Function Label Name: %s\n", name);
				}
			}
			else
			{
				//check if its a variable
				if (strcmp(instruction, "Store") == 0)
				{
					name[strlen(name)-1] = 0;
					if (variables.find(name) == variables.end())
					{
						//add it
						variables[name] = variables.size();
						vars.push_back(0);
					}
				}
				else if (strcmp(instruction, "Load") == 0)
				{
					name[strlen(name)-1] = 0;
					if (variables.find(name) == variables.end())
					{
						//add it
						variables[name] = variables.size();
						vars.push_back(0);
					}
				}
				//printf("Ins: %s at %d\n", instruction, labelposition);
				labelposition++;
			}

			int str = 0;
			while((*code != ';' && *code != ':') || str != 0) 
			{
				if (*code == '\'')
					str = str ? 0 : 1;
				code++;
			}
			code++;
		}

		code = tmp;
		while(*code != 0)
		{
			if (*code == '\n' && *(code+1) == 0)
				break;

			char instruction[20];
			double value;
			sscanf(code, "%s %lf", instruction, &value);
			if (instruction[strlen(instruction)-1] == ';')
			{
				instruction[strlen(instruction)-1] = 0;
			}
			else if (instruction[strlen(instruction)-1] == ':')
			{
				while(*code != ';' && *code != ':') {code++;}
				code++;
				continue;
			}

			Instruction in;
			in.value = value;
			in.value2 = 0;

			if (strcmp(instruction, "LdNum") == 0)
				in.instruction = InstructionType::LdNum;
			else if (strcmp(instruction, "Add") == 0)
				in.instruction = InstructionType::Add;
			else if (strcmp(instruction, "Mul") == 0)
				in.instruction = InstructionType::Mul;
			else if (strcmp(instruction, "Div") == 0)
				in.instruction = InstructionType::Div;
			else if (strcmp(instruction, "Sub") == 0)
				in.instruction = InstructionType::Sub;
			else if (strcmp(instruction, "Mod") == 0)
				in.instruction = InstructionType::Modulus;
			else if (strcmp(instruction, "Decr") == 0)
				in.instruction = InstructionType::Decr;
			else if (strcmp(instruction, "Incr") == 0)
				in.instruction = InstructionType::Incr;
			else if (strcmp(instruction, "Eq") == 0)
				in.instruction = InstructionType::Eq;
			else if (strcmp(instruction, "NotEq") == 0)
				in.instruction = InstructionType::NotEq;
			else if (strcmp(instruction, "Dup") == 0)
				in.instruction = InstructionType::Dup;
			else if (strcmp(instruction, "Lt") == 0)
				in.instruction = InstructionType::Lt;
			else if (strcmp(instruction, "Gt") == 0)
				in.instruction = InstructionType::Gt;
			else if (strcmp(instruction, "Pop") == 0)
				in.instruction = InstructionType::Pop;
			else if (strcmp(instruction, "LdStr") == 0)
			{
				std::string str;
				const char* tmp = code+strlen(instruction)+2;
				if (*code == '\n')
					tmp++;
				const char* tmp2 = tmp;
				while(*tmp2!= '\'') str+= *(tmp2++);
				char* otmp = new char[str.length()+1];
				strcpy(otmp, str.c_str());
				//name[strlen(name)-1] = 0;
				in.string = otmp;
				in.instruction = InstructionType::LdStr;
			}
			else if (strcmp(instruction, "Jmp") == 0)
			{
				char name[50];
				sscanf(code, "%s %s", instruction, name);
				name[strlen(name)-1] = 0;
				in.value = labels[name];
				in.instruction = InstructionType::Jump;
			}
			else if (strcmp(instruction, "JmpTrue") == 0)
			{
				char name[50];
				sscanf(code, "%s %s", instruction, name);
				name[strlen(name)-1] = 0;
				in.value = labels[name];
				in.instruction = InstructionType::JumpTrue;
			}
			else if (strcmp(instruction, "JmpFalse") == 0)
			{
				char name[50];
				sscanf(code, "%s %s", instruction, name);
				name[strlen(name)-1] = 0;
				in.value = labels[name];
				in.instruction = InstructionType::JumpFalse;
			}
			else if (strcmp(instruction, "Store") == 0)
			{
				char name[50];
				sscanf(code, "%s %s", instruction, name);
				name[strlen(name)-1] = 0;
				in.value = variables[name];
				in.instruction = InstructionType::Store;
			}
			else if (strcmp(instruction, "Load") == 0)
			{
				char name[50];
				sscanf(code, "%s %s", instruction, name);
				name[strlen(name)-1] = 0;
				in.value = variables[name];
				in.instruction = InstructionType::Load;
			}
			else if (strcmp(instruction, "LdFn") == 0)
			{
				char name[50];
				sscanf(code, "%s %s", instruction, name);
				name[strlen(name)-1] = 0;
				in.value = functions[name];
				in.instruction = InstructionType::LoadFunction;
			}
			else if (strcmp(instruction, "Call") == 0)
			{
				char name[50]; unsigned int args = 0;
				sscanf(code, "%s %s %d", instruction, name, &args);
				name[strlen(name)] = 0;
				//in.value = variables[name];//labels[name];
				//insert variable with function location
				//check for native functions
				if (variables.find(name) == variables.end())
				{
					//add it
					variables[name] = variables.size();
					vars.push_back(0);

					//vars[variables[name]] = Value(labels[name], true);
					//in.value = variables[name];
					//in.value2 = args;
				}
				else if (vars[variables[name]].type != ValueType::Function && vars[variables[name]].type != ValueType::NativeFunction)
				{
					printf("Warning: Function name used as a variable name: %s\n", name);
				}
				//vars[variables[name]] = Value(labels[name], true);
				in.value = variables[name];
				in.value2 = args;

				//ok, got to make the function here yo
				in.instruction = InstructionType::Call;
			}
			else if (strcmp(instruction, "Return") == 0)
			{
				in.instruction = InstructionType::Return;
			}
			else if (strcmp(instruction, "func") != 0)
			{
				printf("unknown instruction!\n");
			}

			if (strcmp(instruction, "func") != 0)
				ins.push_back(in);

			int str = 0;
			while((*code != ';' && *code != ':') || str != 0) 
			{
				if (*code == '\'')
					str = str ? 0 : 1;
				code++;
			}
			code++;
		}

		QueryPerformanceCounter( (LARGE_INTEGER *)&end );

		INT64 diff = end - start;
		double dt = ((double)diff)/((double)rate);

		printf("Took %lf seconds to assemble\n\n", dt);

		this->callstack.Push(123456789);
		Value temp =  this->Execute(0);//run the static code
		if (this->callstack.size() > 0)
			this->callstack.Pop();

		return temp;
	};

	//executes a function in the VM context
	Value Call(const char* function, Value* args = 0, unsigned int numargs = 0)
	{
		INT64 start, rate, end;
		QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
		QueryPerformanceCounter( (LARGE_INTEGER *)&start );

		int iptr = 0;
		printf("Calling: '%s'\n", function);
		if (variables.find(function) == variables.end())
		{
			printf("ERROR: No variable named: '%s' to call\n", function);
			return Value(0);
		}
		else
		{
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
			iptr = func.ptr;
		}

		//push args onto stack
		for (int i = 0; i < numargs; i++)
		{
			this->stack.Push(args[i]);
		}
		callstack.Push(123456789);//bad value to get it to return;
		Value temp = this->Execute(iptr);
		if (callstack.size() > 0)
			callstack.Pop();
		return temp;
	};

	//these are to be used to return values from native functions
	void Return(Value val)
	{
		this->stack.Push(val);
	}

private:
	Value Execute(int iptr)
	{
		INT64 start, rate, end;
		QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
		QueryPerformanceCounter( (LARGE_INTEGER *)&start );

		try
		{
			int max = ins.size();
			while(iptr < max && iptr >= 0)
			{
				Instruction in = ins[iptr];
				switch(in.instruction)
				{
				case (int)InstructionType::Add:
					{
						Value one = stack.Pop();//stack.top(); stack.pop();
						Value two = stack.Pop();//stack.top(); stack.pop();
						stack.Push(one+two);
						break;
					}
				case (int)InstructionType::Sub:
					{
						Value one = stack.Pop();//stack.top(); stack.pop();
						Value two = stack.Pop();//stack.top(); stack.pop();
						stack.Push(two-one);
						break;
					}
				case (int)InstructionType::Mul:
					{
						Value one = stack.Pop();//stack.top(); stack.pop();
						Value two = stack.Pop();//stack.top(); stack.pop();
						stack.Push(one*two);
						break;
					}
				case (int)InstructionType::Div:
					{
						Value one = stack.Pop();//stack.top(); stack.pop();
						Value two = stack.Pop();//stack.top(); stack.pop();
						stack.Push(two/one);
						break;
					}
				case (int)InstructionType::Modulus:
					{
						Value one = stack.Pop();//stack.top(); stack.pop();
						Value two = stack.Pop();//stack.top(); stack.pop();
						stack.Push(two%one);
						break;
					}
				case (int)InstructionType::Incr:
					{
						Value one = stack.Pop();

						stack.Push(one+Value(1));
						break;
					}
				case (int)InstructionType::Decr:
					{
						Value one = stack.Pop();

						stack.Push(one-Value(1));
						break;
					}
				case (int)InstructionType::Eq:
					{
						Value one = stack.Pop();
						Value two = stack.Pop();

						if ((double)one == (double)two)
							stack.Push(Value(1));
						else
							stack.Push(Value(0));

						break;
					}
				case (int)InstructionType::NotEq:
					{
						Value one = stack.Pop();
						Value two = stack.Pop();

						if ((double)one == (double)two)
							stack.Push(Value(0));
						else
							stack.Push(Value(1));

						break;
					}
				case (int)InstructionType::Lt:
					{
						Value one = stack.Pop();
						Value two = stack.Pop();

						if ((double)one > (double)two)
							stack.Push(Value(1));
						else
							stack.Push(Value(0));

						break;
					}
				case (int)InstructionType::Gt:
					{
						Value one = stack.Pop();
						Value two = stack.Pop();

						if ((double)one < (double)two)
							stack.Push(Value(1));
						else
							stack.Push(Value(0));

						break;
					}
				case (int)InstructionType::LdNum:
					{
						stack.Push(in.value);
						break;
					}
				case (int)InstructionType::LdStr:
					{
						stack.Push(in.string);
						break;
					}
				case (int)InstructionType::Jump:
					{
						iptr = (int)in.value-1;
						break;
					}
				case (int)InstructionType::JumpTrue:
					{
						auto temp = stack.Pop();//stack.top(); stack.pop();
						if ((int)temp)
							iptr = (int)in.value-1;
						break;
					}
				case (int)InstructionType::JumpFalse:
					{
						auto temp = stack.Pop();//stack.top(); stack.pop();
						if (!(int)temp)
							iptr = (int)in.value-1;
						break;
					}
				case (int)InstructionType::Load:
					{
						stack.Push(vars[(int)in.value]);//uh do me
						//auto temp = stack.top(); stack.pop();
						//if (!(int)temp)
						//iptr = (int)in.value-1;
						break;
					}
				case (int)InstructionType::LoadFunction:
					{
						stack.Push(Value((unsigned int)in.value, true));
						break;
					}
				case (int)InstructionType::Store:
					{
						auto temp = stack.Pop();//stack.top(); stack.pop();
						//store me
						vars[(int)in.value] = temp;
						break;
					}
				case (int)InstructionType::Call:
					{
						if (vars[(int)in.value].type == ValueType::Function)
						{
							//store iptr on call stack
							callstack.Push(iptr);
							//go to function
							//lets use labels for now because lazy
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
							//fixme this stuff is baaaad
							callstack.Push(iptr);
							callstack.Push(123456789);
							//to return something, push it to the stack
							int s = stack.size();
							(*vars[(int)in.value].func)(this,tmp,args);

							callstack.QuickPop(2);
							if (stack.size() == s)//we didnt return anything
								stack.Push(Value(0));//dummy return value
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
							throw RuntimeException("Cannot call non function type '" + var + "'!!!\n");
						}

						break;
					}
				case (int)InstructionType::Return:
					{
						//get iptr from stack
						if (callstack.size() == 1)
							iptr = 55555555;
						else
							iptr = callstack.Pop();

						break;
					}
				case (int)InstructionType::Dup:
					{
						stack.Push(stack.Peek());
						break;
					}
				case (int)InstructionType::Pop:
					{
						stack.Pop();
						break;
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

			printf("\nVariables:\n");
			for (auto ii: variables)
			{
				printf("%s = %s\n", ii.first.c_str(), vars[ii.second].ToString().c_str());
			}
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
		}

		if (callstack.size() > 0)
			callstack.Pop();

		QueryPerformanceCounter( (LARGE_INTEGER *)&end );

		INT64 diff = end - start;
		double dt = ((double)diff)/((double)rate);

		printf("Took %lf seconds to execute\n\n", dt);

		/*printf("Variables:\n");
		for (auto ii: variables)
		{
		printf("%s = %s\n", ii.first.c_str(), vars[ii.second].ToString().c_str());
		}

		printf("\nLabels:\n");
		for (auto ii: labels)
		{
		printf("%s = %d\n", ii.first.c_str(), ii.second);
		}*/

		if (stack.size() == 0)
		{
			printf("ERROR: no value to return on stack!\n");
			return Value(0);
		}

		return stack.Pop();
	}

private:
	void StackTrace(int curiptr)
	{
		if (this->callstack.Peek() != 123456789)//dont do this if in native
			this->callstack.Push(curiptr);//push on current location
		while(this->callstack.size() > 0)
		{
			auto top = this->callstack.Pop();
			int greatest = 0;
			std::string fun;
			for (auto ii: this->functions)
			{
				//ok, need to find which label pos I am most greatest than or equal to
				if (top >= ii.second && ii.second > greatest)
				{
					fun = ii.first;
					greatest = ii.second;
				}
			}
			if (greatest == 0)
				printf("{Entry Point+%d}\n", top);
			else if (top == 123456789)
				printf("{Native}\n");
			else
				printf("%s()+%d\n", fun.c_str(), top-greatest);
		}
	}
};
#endif