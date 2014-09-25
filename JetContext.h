#ifndef _LANG_CONTEXT_HEADER
#define _LANG_CONTEXT_HEADER

#include <functional>
#include <string>
#include <map>

#include "Value.h"

#include "VMStack.h"
#include "Parser.h"
#include <Windows.h>

namespace Jet
{
	typedef std::function<void(Jet::JetContext*,Jet::Value*,int)> JetFunction;
#define JetBind(context, fun) 	auto temp__bind_##fun = [](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0]));};context[#fun] = Jet::Value(temp__bind_##fun);
	//void(*temp__bind_##fun)(Jet::JetContext*,Jet::Value*,int)> temp__bind_##fun = &[](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0]));}; context[#fun] = &temp__bind_##fun;

	static enum InstructionType
	{
		Add,
		Mul,
		Div,
		Sub,
		Modulus,

		BAnd,
		BOr,

		Eq,
		NotEq,
		Lt,
		Gt,

		Incr,
		Decr,

		Dup,
		Pop,

		LdNum,
		LdNull,
		LdStr,
		LoadFunction,

		Jump,
		JumpTrue,
		JumpFalse,

		NewArray,
		NewObject,

		Store,
		Load,
		//local vars
		LStore,
		LLoad,

		Close,//closes a range of locals

		//index functions
		LoadAt,
		StoreAt,

		//these all work on the last value in the stack
		EStore,
		ELoad,
		ECall,

		Call,
		Return
	};

	class JetRuntimeException
	{
	public:
		std::string reason;
		JetRuntimeException(std::string reason)
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

	//builtin function definitions
	void gc(JetContext* context,Value* args, int numargs);
	void print(JetContext* context,Value* args, int numargs);

	class JetContext
	{
		VMStack<Value> stack;
		VMStack<unsigned int> callstack;

		//need to go through and find all labels/functions/variables
		::std::map<::std::string, unsigned int> labels;
		::std::map<::std::string, unsigned int> functions;
		::std::map<::std::string, unsigned int> variables;//mapping from string to location in vars array

		//actual data being worked on
		::std::vector<Instruction> ins;
		::std::vector<Value> vars;//where they are actually stored

		//garbage collector stuff
		::std::vector<GCVal<::std::map<int, Value>*>*> arrays;
		::std::vector<GCVal<::std::map<std::string, Value>*>*> objects;

		int labelposition;

		CompilerContext compiler;

	public:
		JetContext()
		{
			this->labelposition = 0;
			this->fptr = -1;
			stack = VMStack<Value>(500000);

			(*this)["print"] = print;
			(*this)["gc"] = gc;
		};

		~JetContext()
		{
			for (auto ii: this->arrays)
			{
				delete ii->ptr;
				delete ii;
			}

			for (auto ii: this->objects)
			{
				delete ii->ptr;
				delete ii;
			}

			for (auto ii: this->ins)
			{
				if (ii.instruction == InstructionType::LdStr || ii.instruction == InstructionType::StoreAt || ii.instruction == InstructionType::LoadAt)
					delete[] ii.string;
			}
		}

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
		const char* Compile(const char* code);

		class StackFrame
		{
		public:
			Value locals[20];//20 max for now

			Value Get(unsigned int id, unsigned int depth)
			{
				if (depth == 0)
					return this->locals[id];
				else
					return (this-depth)->locals[id];
			}

			void Set(Value v, unsigned int id, unsigned int depth)
			{
				if (depth == 0)
					this->locals[id] = v;
				else
					(this-depth)->locals[id] = v;
			}
		};

		//puts compiled code into the VM and runs any globals
		Value Assemble(const char* code)//takes in assembly code for execution
		{
			int startptr = this->ins.size();
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
				sscanf(code, "%s %s", instruction, name);

				if (instruction[0] == '.')
				{
					//comment/debug info
				}
				else if (instruction[strlen(instruction)-1] == ':')//its a label
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
							vars.push_back(Value());
						}
					}
					else if (strcmp(instruction, "Load") == 0)
					{
						name[strlen(name)-1] = 0;
						if (variables.find(name) == variables.end())
						{
							//add it
							variables[name] = variables.size();
							vars.push_back(Value());
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

				if (instruction[0] == '.')//am I a comment?
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
				else if (strcmp(instruction, "ECall") == 0)
					in.instruction = InstructionType::ECall;
				else if (strcmp(instruction, "LoadAt") == 0)
				{
					::std::string str;
					const char* tmp = code+strlen(instruction)+1;
					if (*(code + strlen(instruction)+1) == ' ')
					{
						if (*code == '\n')
							tmp++;
						const char* tmp2 = tmp+1;
						while(*tmp2!= '\'') str+= *(tmp2++);
						char* otmp = new char[str.length()+1];
						strcpy(otmp, str.c_str());
						//name[strlen(name)-1] = 0;
						in.string = otmp;
					}
					else
						in.string = 0;
					in.instruction = InstructionType::LoadAt;
				}
				else if (strcmp(instruction, "StoreAt") == 0)
				{
					::std::string str;
					const char* tmp = code+strlen(instruction)+1;
					if (*(code + strlen(instruction)+1) == ' ')
					{
						if (*code == '\n')
							tmp++;
						const char* tmp2 = tmp+1;
						while(*tmp2!= '\'') str+= *(tmp2++);
						char* otmp = new char[str.length()+1];
						strcpy(otmp, str.c_str());
						//name[strlen(name)-1] = 0;
						in.string = otmp;
					}
					else
						in.string = 0;
					in.instruction = InstructionType::StoreAt;
				}
				else if (strcmp(instruction, "NewArray") == 0)
				{
					int num;
					sscanf(code, "%s %d", instruction, &num);
					in.value = num;
					in.instruction = InstructionType::NewArray;
				}
				else if (strcmp(instruction, "NewObject") == 0)
				{
					int num;
					sscanf(code, "%s %d", instruction, &num);
					in.value = num;
					in.instruction = InstructionType::NewObject;
				}
				else if (strcmp(instruction, "BOR") == 0)
					in.instruction = InstructionType::BOr;
				else if (strcmp(instruction, "BAND") == 0)
					in.instruction = InstructionType::BAnd;
				else if (strcmp(instruction, "LdNull") == 0)
					in.instruction = InstructionType::LdNum;
				else if (strcmp(instruction, "LdStr") == 0)
				{
					::std::string str;
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
				else if (strcmp(instruction, "LStore") == 0)
				{
					int index, depth;
					sscanf(code, "%s %d %d", instruction, &index, &depth);

					in.value = index;
					in.value2 = depth;
					in.instruction = InstructionType::LStore;
				}
				else if (strcmp(instruction, "LLoad") == 0)
				{
					int index, depth;
					sscanf(code, "%s %d %d", instruction, &index, &depth);

					in.value = index;
					in.value2 = depth;
					in.instruction = InstructionType::LLoad;
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
					//insert variable with function location
					//check for native functions
					if (variables.find(name) == variables.end())
					{
						//add it
						variables[name] = variables.size();
						vars.push_back(0);
					}
					else if (vars[variables[name]].type != ValueType::Function && vars[variables[name]].type != ValueType::NativeFunction)
					{
						//printf("Warning: Function name used as a variable name: %s\n", name);
					}
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
			Value temp =  this->Execute(startptr);//run the static code
			if (this->callstack.size() > 0)
				this->callstack.Pop();

			return temp;
		};

		//executes a function in the VM context
		Value Call(const char* function, Value* args = 0, unsigned int numargs = 0)
		{
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
			for (unsigned int i = 0; i < numargs; i++)
			{
				this->stack.Push(args[i]);
			}
			callstack.Push(123456789);//bad value to get it to return;
			Value temp = this->Execute(iptr);
			if (callstack.size() > 0)
				callstack.Pop();
			return temp;
		};

		void RunGC();

		//these are to be used to return values from native functions
		void Return(Value val)
		{
			this->stack.Push(val);
		}

	private:
		StackFrame frames[40];//max call depth
		int fptr;
		Value Execute(int iptr);

	private:
		void StackTrace(int curiptr)
		{
			if (this->callstack.Peek() != 123456789)//dont do this if in native
				this->callstack.Push(curiptr);//push on current location
			while(this->callstack.size() > 0)
			{
				auto top = this->callstack.Pop();
				unsigned int greatest = 0;
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

}
#endif