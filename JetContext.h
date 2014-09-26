#ifndef _LANG_CONTEXT_HEADER
#define _LANG_CONTEXT_HEADER

#include <functional>
#include <string>
#include <map>

#include "Value.h"

#include "VMStack.h"
#include "Parser.h"
#include "JetInstructions.h"
#include <Windows.h>

namespace Jet
{
	typedef std::function<void(Jet::JetContext*,Jet::Value*,int)> JetFunction;
#define JetBind(context, fun) 	auto temp__bind_##fun = [](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0]));};context[#fun] = Jet::Value(temp__bind_##fun);
	//void(*temp__bind_##fun)(Jet::JetContext*,Jet::Value*,int)> temp__bind_##fun = &[](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0]));}; context[#fun] = &temp__bind_##fun;

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
			//const char* string;
		};
		const char* string;
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
		::std::vector<GCVal<char*>*> strings;

		int labelposition;//used for keeping track in assembler
		CompilerContext compiler;//root compiler context

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
				//issue here
				//if (ii.instruction == InstructionType::LdStr || ii.instruction == InstructionType::StoreAt || ii.instruction == InstructionType::LoadAt)
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
			auto asmb = this->Compile(code);
			//this->ins.clear();
			//this->labelposition = 0;
			Value v = this->Assemble(asmb);
			//this->labelposition = 0;
			//this->ins.clear();
			//v = this->Assemble(asmb);
			//delete[] asmb;
			return v;
		}

		//compiles source code to ASM for the VM to read in
		std::vector<IntermediateInstruction> Compile(const char* code);

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

		Value Assemble(const std::vector<IntermediateInstruction>& code)
		{
			int startptr = this->ins.size();
			INT64 start, rate, end;
			QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
			QueryPerformanceCounter( (LARGE_INTEGER *)&start );

			//implement me for supa speed
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
						if (functions.find(inst.string) == functions.end())
							functions[inst.string] = labelposition;
						else
						{
							printf("ERROR: Duplicate Function Label Name: %s\n", inst.string);
						}
						delete[] inst.string;
						break;
					}
				case InstructionType::Label:
					{
						if (this->labels.find(inst.string) == labels.end())
							this->labels[inst.string] = this->labelposition;
						delete[] inst.string;
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
						/*if (functions.find(inst.string) == functions.end())
							functions[inst.string] = labelposition;
						else
						{
							printf("ERROR: Duplicate Function Label Name: %s\n", inst.string);
						}*/
						break;
					}
				case InstructionType::Label:
					{
						/*if (labels.find(inst.string) == labels.end())
							labels[inst.string] = labelposition;
						else
						{
							printf("ERROR: Duplicate Label Name: %s\n", inst.string);
						}*/
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
							ins.value = functions[inst.string];
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

		//debug stuff
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