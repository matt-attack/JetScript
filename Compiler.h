#pragma once

#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <string>
#include <vector>
#include <map>
#include "Token.h"
#include "JetInstructions.h"

namespace Jet
{
	class CompilerException
	{
	public:
		unsigned int line;
		::std::string file;
		::std::string reason;

		CompilerException(std::string file, unsigned int line, std::string r)
		{
			this->line = line;
			this->file = file;
			reason = r;
		};

		const char *ShowReason() const { return reason.c_str(); }
	};

	struct IntermediateInstruction
	{
		InstructionType type;
		/*union
		{
		char* string;
		double second;
		};*/
		char* string;
		double first;
		double second;

		IntermediateInstruction(InstructionType type, const char* string, double num = 0)
		{
			if (string)
			{
				char* c = new char[strlen(string)+1];
				strcpy(c, string);
				this->string = c;
			}
			else 
				this->string = 0;
			//this->string = (char*)string;
			this->type = type;
			this->first = num;
		}

		IntermediateInstruction(InstructionType type, std::string string, double num = 0)
		{
			char* c = new char[string.length()+1];
			strcpy(c, string.c_str());
			this->string = c;
			this->type = type;
			this->second = num;
		}

		IntermediateInstruction(InstructionType type, double num = 0, double num2 = 0)
		{
			this->type = type;
			this->first = num;
			this->second = num2;
			this->string = 0;
		}
	};

	class BlockExpression;

	class CompilerContext
	{

	public:
		::std::vector<IntermediateInstruction> out;

		//::std::string output;
		::std::map<::std::string, CompilerContext*> functions;

		CompilerContext(void);
		~CompilerContext(void);

		CompilerContext* AddFunction(::std::string name)
		{
			//push instruction that sets the function
			//todo, may need to have functions in other instruction code sets

			CompilerContext* newfun = new CompilerContext();
			//insert this into my list of functions
			std::string fname = name+this->GetUUID();
			newfun->uuid = this->uuid;
			this->functions[fname] = newfun;

			//store the function in the variable
			this->LoadFunction(fname);

			return newfun;
		};

		void FinalizeFunction(CompilerContext* c)
		{
			this->uuid += c->uuid + 1;
		}

		std::vector<IntermediateInstruction> Compile(BlockExpression* expr);

	private:
		void Compile()
		{
			//append functions to end here
			for (auto fun: this->functions)
			{
				fun.second->Compile();

				//need to set var with the function name and location
				this->FunctionLabel(fun.first);
				for (auto ins: fun.second->out)
					this->out.push_back(ins);

				//add code of functions recursively
			}
		}

	public:

		struct Scope
		{
			Scope* previous;
			Scope* next;
			int level;
			::std::vector<std::string> localvars;
		};
		Scope* scope;
		void PushScope()
		{
			Scope* s = new Scope;
			this->scope->next = s;
			s->level = this->scope->level + 1;
			s->previous = this->scope;
			s->next = 0;
			this->scope = s;
		}

		void PopScope()
		{
			if (this->scope && this->scope->previous)
				this->scope = this->scope->previous;

			delete this->scope->next;
			this->scope->next = 0;
		}

		struct _LoopInfo
		{
			std::string Break;
			std::string Continue;
		};
		std::vector<_LoopInfo> _loops;
		void PushLoop(std::string Break, std::string Continue)
		{
			_LoopInfo i;
			i.Break = Break;
			i.Continue = Continue;
			_loops.push_back(i);
		}

		void PopLoop()
		{
			_loops.pop_back();
		}

		void Break()
		{
			if (this->_loops.size() == 0)
				throw new CompilerException("test", 0, "Cannot use break outside of a loop!");
			this->Jump(_loops.back().Break.c_str());
		}

		void Continue()
		{
			if (this->_loops.size() == 0)
				throw new CompilerException("test", 0, "Cannot use continue outside of a loop!");
			this->Jump(_loops.back().Continue.c_str());
		}

		bool RegisterLocal(std::string name);//returns success

		void BinaryOperation(TokenType operation);
		void UnaryOperation(TokenType operation);

		//stack operations
		void Pop()
		{
			out.push_back(IntermediateInstruction(InstructionType::Pop));
		}

		void Duplicate()
		{
			out.push_back(IntermediateInstruction(InstructionType::Dup));
		}

		//load operations
		void Null()
		{
			out.push_back(IntermediateInstruction(InstructionType::LdNull));
		}
		void Number(double value)
		{
			out.push_back(IntermediateInstruction(InstructionType::LdNum, value));
		}

		void String(std::string string)
		{
			out.push_back(IntermediateInstruction(InstructionType::LdStr, string.c_str()));
		}

		void JumpFalse(const char* pos)
		{
			out.push_back(IntermediateInstruction(InstructionType::JumpFalse, pos));
		}

		void JumpTrue(const char* pos)
		{
			out.push_back(IntermediateInstruction(InstructionType::JumpTrue, pos));
		}

		void Jump(const char* pos)
		{
			out.push_back(IntermediateInstruction(InstructionType::Jump, pos));
		}

		void FunctionLabel(std::string name)
		{
			out.push_back(IntermediateInstruction(InstructionType::Function, name));
		}

		void Label(std::string name)
		{
			out.push_back(IntermediateInstruction(InstructionType::Label, name));
		}

		void Store(std::string variable)
		{
			//look up if I am a local or global
			Scope* ptr = this->scope;
			while (ptr)
			{
				//look for var in locals
				for (unsigned int i = 0; i < ptr->localvars.size(); i++)
				{
					if (ptr->localvars[i] == variable)
					{
						//printf("We found storing of a local var: %s at level %d\n", variable.c_str(), ptr->level);
						//exit the loops we found it
						//this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
						out.push_back(IntermediateInstruction(InstructionType::LStore, i, ptr->level));
						return;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}
			out.push_back(IntermediateInstruction(InstructionType::Store, variable));
		}

		void StoreLocal(std::string variable)
		{
			//look up if I am local or global
			this->Store(variable);
		}

		//this loads locals and globals atm
		void Load(std::string variable)
		{
			Scope* ptr = this->scope;
			while (ptr)
			{
				//look for var in locals
				for (unsigned int i = 0; i < ptr->localvars.size(); i++)
				{
					if (ptr->localvars[i] == variable)
					{
						//printf("We found loading of a local var: %s at level %d\n", variable.c_str(), ptr->level);
						//exit the loops we found it
						//comment/debug info
						//this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
						out.push_back(IntermediateInstruction(InstructionType::LLoad, i, ptr->level));
						return;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}
			out.push_back(IntermediateInstruction(InstructionType::Load, variable));
		}


		void LoadFunction(::std::string name)
		{
			out.push_back(IntermediateInstruction(InstructionType::LoadFunction, name));
		}

		void Call(::std::string function, unsigned int args)
		{
			out.push_back(IntermediateInstruction(InstructionType::Call, function, args));
		}

		void ECall(unsigned int args)
		{
			out.push_back(IntermediateInstruction(InstructionType::ECall, args));
		}

		void LoadIndex(const char* index = 0)
		{
			out.push_back(IntermediateInstruction(InstructionType::LoadAt, index));
		}

		void StoreIndex(const char* index = 0)
		{
			out.push_back(IntermediateInstruction(InstructionType::StoreAt, index));
		}

		void NewArray(unsigned int number)
		{
			out.push_back(IntermediateInstruction(InstructionType::NewArray, number));
		}

		void NewObject(unsigned int number)
		{
			out.push_back(IntermediateInstruction(InstructionType::NewObject, number));
		}

		void Return()
		{
			out.push_back(IntermediateInstruction(InstructionType::Return));
		}

	private:
		int uuid;

	public:
		::std::string GetUUID()//use for autogenerated labels
		{
			return ::std::to_string(uuid++);
		}
	};

};