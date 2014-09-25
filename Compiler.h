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

namespace Jet
{
	class BlockExpression;

	class CompilerContext
	{

	public:
		::std::string output;
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

		std::string Compile(BlockExpression* expr);

	private:
		void Compile()
		{
			//append functions to end here
			for (auto fun: this->functions)
			{
				this->output += "\n\n";

				fun.second->Compile();

				//need to set var with the function name and location
				this->FunctionLabel(fun.first);
				this->output += fun.second->output;

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
			output += "Pop;\n";
		}

		void Duplicate()
		{
			output += "Dup;\n";
		}

		//load operations
		void Null()
		{
			this->output += "LdNull;\n";
		}
		void Number(double value)
		{
			char t[50];
			sprintf(t, "LdNum %lf;\n", value);
			this->output += t;
		}

		void String(std::string string)
		{
			this->output += "LdStr '"+string+"';\n";
		}

		void JumpFalse(const char* pos)
		{
			char t[50];
			sprintf(t, "JmpFalse %s;\n", pos);
			this->output += t;
		}

		void JumpTrue(const char* pos)
		{
			char t[50];
			sprintf(t, "JmpTrue %s;\n", pos);
			this->output += t;
		}

		void Jump(const char* pos)
		{
			char t[50];
			sprintf(t, "Jmp %s;\n", pos);
			this->output += t;
		}

		void FunctionLabel(std::string name)
		{
			this->output += "func " + name + ":\n";
		}

		void Label(std::string name)
		{
			this->output += name + ":\n";
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
						this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
						this->output += "LStore " + ::std::to_string(i) + " " + ::std::to_string(ptr->level) + ";\n";
						return;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}
			this->output += "Store " + variable + ";\n";
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
						this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
						this->output += "LLoad " + ::std::to_string(i) +" " + ::std::to_string(ptr->level) + ";\n";
						return;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}
			this->output += "Load " + variable + ";\n";
		}


		void LoadFunction(::std::string name)
		{
			this->output += "LdFn " + name + ";\n";
		}

		void Call(::std::string function, unsigned int args)
		{
			this->output += "Call " + function + " " + ::std::to_string(args) + ";\n";
		}

		void ECall(unsigned int args)
		{
			this->output += "ECall " + ::std::to_string(args) + ";\n";
		}

		void LoadIndex(const char* index = 0)
		{
			if (index)
				this->output += "LoadAt '"+std::string(index)+"';\n";
			else
				this->output += "LoadAt;\n";
		}

		void StoreIndex(const char* index = 0)
		{
			if (index)
				this->output += "StoreAt '"+std::string(index)+"';\n";
			else
				this->output += "StoreAt;\n";
		}

		void NewArray(unsigned int number)
		{
			this->output += "NewArray " + std::to_string(number) + ";\n";
		}

		void NewObject(unsigned int number)
		{
			this->output += "NewObject " + std::to_string(number) + ";\n";
		}

		void Return()
		{
			this->output += "Return;\n";
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