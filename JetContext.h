#ifndef _LANG_CONTEXT_HEADER
#define _LANG_CONTEXT_HEADER

#include <functional>
#include <string>
#include <map>
#include <algorithm>

#include "Value.h"

#include "VMStack.h"
#include "Parser.h"
#include "JetInstructions.h"
#include "JetExceptions.h"


#ifdef _WIN32
#include <Windows.h>
//#define JET_TIME_EXECUTION
#endif

namespace Jet
{
	typedef std::function<void(Jet::JetContext*,Jet::Value*,int)> JetFunction;
#define JetBind(context, fun) 	auto temp__bind_##fun = [](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0]));};context[#fun] = Jet::Value(temp__bind_##fun);
	//void(*temp__bind_##fun)(Jet::JetContext*,Jet::Value*,int)> temp__bind_##fun = &[](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0]));}; context[#fun] = &temp__bind_##fun;

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

		//debug info
		struct FunctionData
		{
			std::string file;
			unsigned int line;
			unsigned int ptr;
			unsigned int locals;
		};
		::std::map<::std::string, FunctionData> dfunctions;

		struct DebugInfo
		{
			unsigned int code;
			std::string file;
			unsigned int line;
		};
		std::vector<DebugInfo> debuginfo;
		//::std::map<unsigned int, std::pair<std::string, unsigned int>> debuginfo;

		//actual data being worked on
		::std::vector<Instruction> ins;
		::std::vector<Value> vars;//where they are actually stored

		//garbage collector stuff
		::std::vector<GCVal<::std::map<int, Value>*>*> arrays;
		::std::vector<GCVal<::std::map<std::string, Value>*>*> objects;
		::std::vector<GCVal<char*>*> strings;
		::std::vector<_JetUserdata*> userdata;

		::std::vector<char*> gcObjects;

		int labelposition;//used for keeping track in assembler
		CompilerContext compiler;//root compiler context

		_JetObject string;
		_JetObject Array;
		_JetObject file;
	public:

		Value NewObject()
		{
			auto v = new _JetObject;
			return Value(v);
		}

		Value NewUserdata(void* data, _JetObject* proto)
		{
			auto ud = new GCVal<std::pair<void*, _JetObject*>>(std::pair<void*, _JetObject*>(data, proto));
			this->userdata.push_back(ud);
			return Value(ud, proto);
		}

		Value NewString(char* string)
		{
			this->strings.push_back(new GCVal<char*>(string));
			return Value(string);
		}

	private:

		//must free with GCFree, pointer is a bit offset to leave room for the flag
		template<class T> 
		T* GCAllocate()
		{
#undef new
			//need to call constructor
			char* buf = new char[sizeof(T)+1];
			this->gcObjects.push_back(buf);
			new (buf+1) T();
			return (T*)(buf+1);

#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif
		}

		template<class T> 
		T* GCAllocate2(unsigned int size)
		{
			return (T*)((new char[sizeof(T)+1])+1);
		}

		char* GCAllocate(unsigned int size)
		{
			//this leads to less indirection, and simplified cleanup
			char* data = new char[size+1];//enough room for the flag
			this->gcObjects.push_back(data);
			return data+1;
		}

		//need to call destructor first
		template<class T>
		void GCFree(T* data)
		{
			data->~T();
			delete[] (((char*)data)-1);
		}

		void GCFree(char* data)
		{
			delete[] (data-1);
		}

	public:

		JetContext()
		{
			this->labelposition = 0;
			this->fptr = -1;
			stack = VMStack<Value>(500000);
			//add more functions and junk
			(*this)["print"] = print;
			(*this)["gc"] = gc;
			(*this)["setprototype"] = Value([](JetContext* context, Value* v, int args)
			{
				if (args != 2)
					throw JetRuntimeException("Invalid Call, Improper Arguments!");

				if (v->type == ValueType::Object && (v+1)->type == ValueType::Object)
				{
					Value val = *v;
					val._obj.prototype = (v+1)->_obj._object;
					context->Return(val);
				}
				else
				{
					throw JetRuntimeException("Improper arguments!");
				}
			});


			this->file.ptr = new std::map<std::string, Value>;
			(*file.ptr)["read"] = Value([](JetContext* context, Value* v, int args)
			{
				if (args != 2)
					throw JetRuntimeException("Invalid number of arguments to read!");

				char* out = new char[((int)v->value)+1];//context->GCAllocate((v)->value);
				fread(out, 1, (int)(v)->value, (v+1)->GetUserdata<FILE>());
				out[(int)(v)->value] = 0;
				context->Return(context->NewString(out));
			});
			(*file.ptr)["write"] = Value([](JetContext* context, Value* v, int args)
			{
				if (args != 2)
					throw JetRuntimeException("Invalid number of arguments to write!");

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
					throw JetRuntimeException("Invalid number of arguments to fopen!");

				FILE* f = fopen(v->ToString().c_str(), (*(v+1)).ToString().c_str());
				context->Return(context->NewUserdata(f, &context->file));
			});

			(*this)["fclose"] = Value([](JetContext* context, Value* v, int args)
			{
				if (args != 1)
					throw JetRuntimeException("Invalid number of arguments to fclose!");
				fclose(v->GetUserdata<FILE>());
			});


			//setup the string and array tables
			this->string.ptr = new std::map<std::string, Value>;
			(*this->string.ptr)["append"] = Value([](JetContext* context, Value* v, int args)
			{
				if (args == 2)
					context->Return(Value("newstr"));
				else
					throw JetRuntimeException("bad append call!");
			});
			(*this->string.ptr)["length"] = Value([](JetContext* context, Value* v, int args)
			{
				if (args == 1)
					context->Return(Value((double)strlen(v->_obj._string)));
				else
					throw JetRuntimeException("bad length call!");
			});

			this->Array.ptr = new std::map<std::string, Value>;
			(*this->Array.ptr)["add"] = Value([](JetContext* context, Value* v, int args)
			{
				if (args == 2)
					(*(v+1)->_obj._array->ptr)[(v+1)->_obj._array->ptr->size()] = *(v);
				else
					throw JetRuntimeException("Invalid add call!!");
			});
			(*this->Array.ptr)["size"] = Value([](JetContext* context, Value* v, int args)
			{
				//how do I get access to the array from here?
				if (args == 1)
					context->Return((int)v->_obj._array->ptr->size());
				else
					throw JetRuntimeException("Invalid size call!!");
			});
		};

		~JetContext()
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
						throw JetRuntimeException("Not Implemented!");
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

			delete this->string.ptr;
			delete this->Array.ptr;
			delete this->file.ptr;
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

			Value v = this->Assemble(asmb);

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
#ifdef JET_TIME_EXECUTION
			INT64 start, rate, end;
			QueryPerformanceFrequency( (LARGE_INTEGER *)&rate );
			QueryPerformanceCounter( (LARGE_INTEGER *)&start );
#endif

			//implement me for supa speed
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
						if (functions.find(inst.string) == functions.end())
							functions[inst.string] = labelposition;
						else
							throw JetRuntimeException("ERROR: Duplicate Function Label Name: %s\n" + std::string(inst.string));

						delete[] inst.string;
						break;
					}
				case InstructionType::Label:
					{
						if (this->labels.find(inst.string) == labels.end())
							this->labels[inst.string] = this->labelposition;
						else
							throw JetRuntimeException("ERROR: Duplicate Label Name: %s\n" + std::string(inst.string));
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

#ifdef JET_TIME_EXECUTION
			QueryPerformanceCounter( (LARGE_INTEGER *)&end );

			INT64 diff = end - start;
			double dt = ((double)diff)/((double)rate);

			printf("Took %lf seconds to assemble\n\n", dt);
#endif

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
		int fptr;
		StackFrame frames[40];//max call depth
		Value Execute(int iptr);

		//debug stuff
		void GetCode(int ptr, std::string& ret, unsigned int& line)
		{
			//std::binary_search(this->debuginfo.begin(), this->debuginfo.end(), line);
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
			int index = min(imin, imax);
			ret = this->debuginfo[index].file;
			line = this->debuginfo[index].line;
			/*for (int i = this->debuginfo.size()-1; i >= 0; i--)
			{
				DebugInfo info = this->debuginfo[i];
				if (info.code <= ptr)
				{
					//fix me line numbers are REALLY wrong
					ret = info.file;
					line = info.line;
					break;
				}
			}*/
		}

		void StackTrace(int curiptr)
		{
			VMStack<unsigned int> tempcallstack = this->callstack.Copy();
			tempcallstack.Push(curiptr);

			while(tempcallstack.size() > 0)
			{
				auto top = tempcallstack.Pop();
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
				if (top == 123456789)
					printf("{Native}\n");
				else if (greatest == 0)//fixme
				{
					std::string file;
					unsigned int line;
					this->GetCode(top, file, line);
					printf("{Entry Point+%d} %s Line %d\n", top, file.c_str(), line);
				}
				else
				{
					//finish me
					std::string file;
					unsigned int line;
					this->GetCode(top, file, line);
					//printf("File: %s line %d ", file.c_str(), line);
					printf("%s() %s Line %d (Instruction %d)\n", fun.c_str(), file.c_str(), line, top-greatest);
				}
			}
		}
	};
}
#endif