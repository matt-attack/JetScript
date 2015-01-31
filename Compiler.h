#pragma once

#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <string>
#include <vector>
#include <map>

#include "Token.h"
#include "JetInstructions.h"
#include "JetExceptions.h"


namespace Jet
{
	//add custom operators
	//add includes/modules
	//parallelism maybe?
	//add const
	struct IntermediateInstruction
	{
		InstructionType type;

		char* string;
		int first;
		union
		{
			double second;
			struct 
			{
				unsigned char a,b,c,d;
			};
		};

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

			this->second = 0;
			this->type = type;
			this->first = num;
		}

		IntermediateInstruction(InstructionType type, std::string string, double num = 0)
		{
			char* c = new char[string.length()+1];
			strcpy(c, string.c_str());
			this->string = c;
			this->type = type;
			this->first = 0;
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

	template <class T, class T2, class T3>
	struct triple
	{
		T first;
		T2 second;
		T3 third;

		triple(T t, T2 t2, T3 t3) : first(t), second(t2), third(t3)
		{

		};
	};

	class CompilerContext
	{
		friend class CallExpression;
		std::map<std::string, CompilerContext*> functions;

		struct LoopInfo
		{
			std::string Break;
			std::string Continue;
		};
		std::vector<LoopInfo> loops;

		struct LocalVariable
		{
			LocalVariable() { this->uploaded = false; }
			int local;
			std::string name;
			int capture;
			bool uploaded;
		};

		struct Scope
		{
			Scope* previous;
			Scope* next;
			int level;
			std::vector<LocalVariable> localvars;
		};
		Scope* scope;

		unsigned int localindex;

		bool vararg;
		unsigned int closures;
		unsigned int arguments;
		CompilerContext* parent;

		std::vector<IntermediateInstruction> out;

	public:

		CompilerContext(void);
		~CompilerContext(void);

		void PrintAssembly();

		//used when generating functions
		CompilerContext* AddFunction(std::string name, unsigned int args, bool vararg = false);
		void FinalizeFunction(CompilerContext* c);

		std::vector<IntermediateInstruction> Compile(BlockExpression* expr);

	private:
		void Compile()
		{
			//append functions to end here
			for (auto fun: this->functions)
			{
				fun.second->Compile();

				//need to set var with the function name and location
				this->FunctionLabel(fun.first, fun.second->arguments, fun.second->localindex, fun.second->closures);
				for (auto ins: fun.second->out)
					this->out.push_back(ins);

				//add code of functions recursively
			}
		}

		void FunctionLabel(std::string name, int args, int locals, int upvals, bool vararg = false)
		{
			IntermediateInstruction ins = IntermediateInstruction(InstructionType::Function, name, args);
			ins.a = args;
			ins.b = locals;
			ins.c = upvals;
			ins.d = vararg ? 1 : 0;
			out.push_back(ins);
		}

	public:

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

			if (this->scope)
			{
				delete this->scope->next;
				this->scope->next = 0;
			}
		}

		void PushLoop(const std::string Break, const std::string Continue)
		{
			LoopInfo i;
			i.Break = Break;
			i.Continue = Continue;
			loops.push_back(i);
		}

		void PopLoop()
		{
			//do a close if necessary
			/*for (auto& ii : this->scope->localvars)
			{
				if (ii.capture >= 0)
				{
					out.push_back(IntermediateInstruction(InstructionType::CInit,ii.local, ii.capture));
					out.push_back(IntermediateInstruction(InstructionType::Close));
					ii.uploaded = true;
				}
			}*/
			loops.pop_back();
		}

		void Break()
		{
			if (this->loops.size() == 0)
				throw CompilerException(this->lastfile, this->lastline, "Cannot use break outside of a loop!");
			this->Jump(loops.back().Break.c_str());
		}

		void Continue()
		{
			if (this->loops.size() == 0)
				throw CompilerException(this->lastfile, this->lastline, "Cannot use continue outside of a loop!");
			this->Jump(loops.back().Continue.c_str());
		}

		bool RegisterLocal(const std::string name);//returns success

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
			out.push_back(IntermediateInstruction(InstructionType::LdNum, (double)0, value));
		}

		void String(std::string string)
		{
			out.push_back(IntermediateInstruction(InstructionType::LdStr, string.c_str()));
		}

		//jumps
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

		void Label(const std::string name)
		{
			out.push_back(IntermediateInstruction(InstructionType::Label, name));
		}

		void Store(const std::string variable)
		{
			//look up if I am a local or global
			Scope* ptr = this->scope;
			while (ptr)
			{
				//look for var in locals
				for (unsigned int i = 0; i < ptr->localvars.size(); i++)
				{
					if (ptr->localvars[i].name == variable)
					{
						//printf("We found storing of a local var: %s at level %d, index %d\n", variable.c_str(), ptr->level, ptr->localvars[i].first);
						//exit the loops we found it
						//this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
						out.push_back(IntermediateInstruction(InstructionType::LStore, ptr->localvars[i].local, 0));//i, ptr->level));
						return;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}

			int level = -1;
			auto cur = this->parent;
			while(cur)//if (this->parent)
			{
				ptr = cur->scope;
				while (ptr)
				{
					//look for var in locals
					for (unsigned int i = 0; i < ptr->localvars.size(); i++)
					{
						if (ptr->localvars[i].name == variable)
						{
							//printf("We found storing of a captured var: %s at level %d, index %d\n", variable.c_str(), level, ptr->localvars[i].first);
							//exit the loops we found it
							//this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
							if (ptr->localvars[i].capture == -1)
								ptr->localvars[i].capture = cur->closures++;

							out.push_back(IntermediateInstruction(InstructionType::CStore, ptr->localvars[i].capture, level));//i, ptr->level));
							return;
						}
					}
					if (ptr)
						ptr = ptr->previous;
				}
				level--;
				cur = cur->parent;
			}
			out.push_back(IntermediateInstruction(InstructionType::Store, variable));
		}

		void StoreLocal(const std::string variable)
		{
			//look up if I am local or global
			this->Store(variable);
		}

		//this loads locals and globals atm
		void Load(const std::string variable)
		{
			Scope* ptr = this->scope;
			while (ptr)
			{
				//look for var in locals
				for (unsigned int i = 0; i < ptr->localvars.size(); i++)
				{
					if (ptr->localvars[i].name == variable)
					{
						//printf("We found loading of a local var: %s at level %d, index %d\n", variable.c_str(), ptr->level, ptr->localvars[i].first);
						//exit the loops we found it
						//comment/debug info
						//this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
						out.push_back(IntermediateInstruction(InstructionType::LLoad, ptr->localvars[i].local, 0));//i, ptr->level));
						return;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}

			int level = -1;
			auto cur = this->parent;
			while(cur)//if (this->parent)
			{
				ptr = cur->scope;
				while (ptr)
				{
					//look for var in locals
					for (unsigned int i = 0; i < ptr->localvars.size(); i++)
					{
						if (ptr->localvars[i].name == variable)
						{
							//printf("We found loading of a captured var: %s at level %d, index %d\n", variable.c_str(), level, ptr->localvars[i].first);
							//exit the loops we found it
							//comment/debug info
							//this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
							if (ptr->localvars[i].capture == -1)
								ptr->localvars[i].capture = cur->closures++;

							out.push_back(IntermediateInstruction(InstructionType::CLoad, ptr->localvars[i].capture, level));//i, ptr->level));
							return;
						}
					}
					if (ptr)
						ptr = ptr->previous;
				}
				level--;
				cur = cur->parent;
			}
			out.push_back(IntermediateInstruction(InstructionType::Load, variable));
		}

		bool IsLocal(const std::string variable)
		{
			Scope* ptr = this->scope;
			while (ptr)
			{
				//look for var in locals
				for (unsigned int i = 0; i < ptr->localvars.size(); i++)
				{
					if (ptr->localvars[i].name == variable)
					{
						//printf("We found loading of a local var: %s at level %d, index %d\n", variable.c_str(), ptr->level, ptr->localvars[i].first);
						//exit the loops we found it
						//comment/debug info
						//this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";
						return true;
					}
				}
				if (ptr)
					ptr = ptr->previous;
			}

			auto cur = this->parent;
			while(cur)//if (this->parent)
			{
				ptr = cur->scope;
				while (ptr)
				{
					//look for var in locals
					for (unsigned int i = 0; i < ptr->localvars.size(); i++)
					{
						if (ptr->localvars[i].name == variable)
						{
							//printf("We found loading of a captured var: %s at level %d, index %d\n", variable.c_str(), level, ptr->localvars[i].first);
							//exit the loops we found it
							//comment/debug info
							//this->output += ".local " + variable + " " + ::std::to_string(i) + ";\n";

							return true;
						}
					}
					if (ptr)
						ptr = ptr->previous;
				}
				cur = cur->parent;
			}
			return false;
		}

		void LoadFunction(const std::string name)
		{
			out.push_back(IntermediateInstruction(InstructionType::LoadFunction, name));
		}

		void Call(const std::string function, unsigned int args)
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
			if (this->closures > 0)//close all open closures
				out.push_back(IntermediateInstruction(InstructionType::Close));
			out.push_back(IntermediateInstruction(InstructionType::Return));
		}

		//debug info
		std::string lastfile; unsigned int lastline;
		void Line(const std::string file, unsigned int line)
		{
			//need to avoid duplicates, because thats silly
			if (lastline != line || lastfile != file)
			{
				lastline = line; lastfile = file;
				out.push_back(IntermediateInstruction(InstructionType::DebugLine, file, line));
			}
		}

	private:
		int uuid;

	public:
		std::string GetUUID()//use for autogenerated labels
		{
			return std::to_string(uuid++);
		}
	};

};