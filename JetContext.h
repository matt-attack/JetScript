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

#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#ifdef _WIN32
#include <Windows.h>
//#define JET_TIME_EXECUTION
#endif

#define GC_INTERVAL 100//number of allocations before running the GC
#define GC_STEPS 4//number of incremental runs before a full

namespace Jet
{
	typedef std::function<void(Jet::JetContext*,Jet::Value*,int)> JetFunction;
#define JetBind(context, fun) 	auto temp__bind_##fun = [](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0]));};context[#fun] = Jet::Value(temp__bind_##fun);
	//void(*temp__bind_##fun)(Jet::JetContext*,Jet::Value*,int)> temp__bind_##fun = &[](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0]));}; context[#fun] = &temp__bind_##fun;
#define JetBind2(context, fun) 	auto temp__bind_##fun = [](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun(args[0],args[1]));};context[#fun] = Jet::Value(temp__bind_##fun);
#define JetBind2(context, fun, type) 	auto temp__bind_##fun = [](Jet::JetContext* context,Jet::Value* args, int numargs) { context->Return(fun((type)args[0],(type)args[1]));};context[#fun] = Jet::Value(temp__bind_##fun);

	struct Instruction
	{
		InstructionType instruction;
		union
		{
			double value;
			Function* func;
		};
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
		VMStack<std::pair<unsigned int, Closure*> > callstack;

		//need to go through and find all labels/functions/variables
		::std::map<::std::string, unsigned int> labels;
		::std::map<::std::string, Function*> functions;
		::std::vector<Function*> entrypoints;
		::std::map<::std::string, unsigned int> variables;//mapping from string to location in vars array

		//debug info
		struct DebugInfo
		{
			unsigned int code;
			std::string file;
			unsigned int line;
		};
		std::vector<DebugInfo> debuginfo;

		//actual data being worked on
		::std::vector<Instruction> ins;
		::std::vector<Value> vars;//where they are actually stored

		int labelposition;//used for keeping track in assembler
		CompilerContext compiler;//root compiler context

		_JetObject string;
		_JetObject Array;
		_JetObject object;
		_JetObject file;
		_JetObject arrayiter;
		_JetObject objectiter;

	private:
		//garbage collector stuff
		::std::vector<GCVal<::std::vector<Value>*>*> arrays;
		::std::vector<GCVal<::std::map<std::string, Value>*>*> objects;
		::std::vector<GCVal<char*>*> strings;
		::std::vector<_JetUserdata*> userdata;

		::std::vector<Closure*> closures;

		::std::vector<char*> gcObjects;

		
		int allocationCounter;//used to determine when to run the GC
		int collectionCounter;
		VMStack<Value> greys;//stack of grey objects for processing
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

#ifdef _DEBUG
#ifndef DBG_NEW      
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )     
#define new DBG_NEW   
#endif
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

		JetContext();
		~JetContext();

		//allows assignment and reading of variables stored
		Value& operator[](const char* id);

		std::string Script(const std::string code, const std::string filename = "file");

		Value Script(const char* code, const char* filename = "file");//compiles, assembles and executes the script


		//compiles source code to ASM for the VM to read in
		std::vector<IntermediateInstruction> Compile(const char* code, const char* filename = "file");

		struct StackFrame
		{
			int size;
			Value locals[20];//20 max for now
		};

		Value Assemble(const std::vector<IntermediateInstruction>& code);

		//executes a function in the VM context
		Value Call(const char* function, Value* args = 0, unsigned int numargs = 0);

		void RunGC();
		void RunGC2();

		//these are to be used to return values from native functions
		void Return(Value val)
		{
			this->stack.Push(val);
		}

	private:
		int fptr;
		Closure* curframe;
		StackFrame frames[40];//max call depth
		Value Execute(int iptr);

		//debug stuff
		void GetCode(int ptr, std::string& ret, unsigned int& line);

		void StackTrace(int curiptr);
	};

}

#endif