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
				//this->output += "\n\n";

				fun.second->Compile();

				//need to set var with the function name and location
				this->FunctionLabel(fun.first);
				for (auto ins: fun.second->out)
					this->out.push_back(ins);
				//this->output += fun.second->output;

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
			//printf("scope pushed\n");
		}

		void PopScope()
		{
			if (this->scope && this->scope->previous)
				this->scope = this->scope->previous;

			delete this->scope->next;
			this->scope->next = 0;
		}

		bool RegisterLocal(std::string name);//returns success

		void BinaryOperation(TokenType operation);
		void UnaryOperation(TokenType operation);

		//stack operations
		void Pop()
		{
			out.push_back(IntermediateInstruction(InstructionType::Pop));
			//output += "Pop;\n";
		}

		void Duplicate()
		{
			out.push_back(IntermediateInstruction(InstructionType::Dup));
			//output += "Dup;\n";
		}

		//load operations
		void Null()
		{
			out.push_back(IntermediateInstruction(InstructionType::LdNull));
			//this->output += "LdNull;\n";
		}
		void Number(double value)
		{
			out.push_back(IntermediateInstruction(InstructionType::LdNum, value));
			char t[50];
			sprintf(t, "LdNum %lf;\n", value);
			//this->output += t;
		}

		void String(std::string string)
		{
			out.push_back(IntermediateInstruction(InstructionType::LdStr, string.c_str()));
			//this->output += "LdStr '"+string+"';\n";
		}

		void JumpFalse(const char* pos)
		{
			out.push_back(IntermediateInstruction(InstructionType::JumpFalse, pos));
			//char t[50];
			//sprintf(t, "JmpFalse %s;\n", pos);
			//this->output += t;
		}

		void JumpTrue(const char* pos)
		{
			out.push_back(IntermediateInstruction(InstructionType::JumpTrue, pos));
			/*char t[50];
			sprintf(t, "JmpTrue %s;\n", pos);
			this->output += t;*/
		}

		void Jump(const char* pos)
		{
			out.push_back(IntermediateInstruction(InstructionType::Jump, pos));
			/*char t[50];
			sprintf(t, "Jmp %s;\n", pos);
			this->output += t;*/
		}

		void FunctionLabel(std::string name)
		{
			out.push_back(IntermediateInstruction(InstructionType::Function, name));

			//this->output += "func " + name + ":\n";
		}

		void Label(std::string name)
		{
			out.push_back(IntermediateInstruction(InstructionType::Label, name));

			//this->output += name + ":\n";
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
						//this->output += "LStore " + ::std::to_string(i) + " " + ::std::to_string(ptr->level) + ";\n";
						return;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}
			out.push_back(IntermediateInstruction(InstructionType::Store, variable));
			//this->output += "Store " + variable + ";\n";
		}

		void StoreLocal(std::string variable)
		{
			//look up if I am local or global
			this->Store(variable);
			//this->output += "Store " + variable + ";\n";
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
						//this->output += "LLoad " + ::std::to_string(i) +" " + ::std::to_string(ptr->level) + ";\n";
						return;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}
			out.push_back(IntermediateInstruction(InstructionType::Load, variable));
			//this->output += "Load " + variable + ";\n";
		}


		void LoadFunction(::std::string name)
		{
			out.push_back(IntermediateInstruction(InstructionType::LoadFunction, name));
			//this->output += "LdFn " + name + ";\n";
		}

		void Call(::std::string function, unsigned int args)
		{
			out.push_back(IntermediateInstruction(InstructionType::Call, function, args));
			//this->output += "Call " + function + " " + ::std::to_string(args) + ";\n";
		}

		void ECall(unsigned int args)
		{
			out.push_back(IntermediateInstruction(InstructionType::ECall, args));
			//this->output += "ECall " + ::std::to_string(args) + ";\n";
		}

		void LoadIndex(const char* index = 0)
		{
			out.push_back(IntermediateInstruction(InstructionType::LoadAt, index));

			/*if (index)
				this->output += "LoadAt '"+std::string(index)+"';\n";
			else
				this->output += "LoadAt;\n";*/
		}

		void StoreIndex(const char* index = 0)
		{
			out.push_back(IntermediateInstruction(InstructionType::StoreAt, index));

			/*if (index)
				this->output += "StoreAt '"+std::string(index)+"';\n";
			else
				this->output += "StoreAt;\n";*/
		}

		void NewArray(unsigned int number)
		{
			out.push_back(IntermediateInstruction(InstructionType::NewArray, number));
			//this->output += "NewArray " + std::to_string(number) + ";\n";
		}

		void NewObject(unsigned int number)
		{
			out.push_back(IntermediateInstruction(InstructionType::NewObject, number));
			//this->output += "NewObject " + std::to_string(number) + ";\n";
		}

		void Return()
		{
			out.push_back(IntermediateInstruction(InstructionType::Return));
			//this->output += "Return;\n";
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